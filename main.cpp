/**
 * NF_Tx_Core_Tester — the single front-end for running the core
 * =================================================================
 * Two ways to run it:
 *
 * 1) argv CLI (the former NF_Tx_Core_WalletDump surface, unchanged):
 *
 *    NF_Tx_Core_Tester [path/to/libNF_Tx_Core.so] <csv-file> [mode] [out-file] [date] [--extended]
 *
 *      so-path    optional; default: first existing of {
 *          ../../Nf_Tx_Core/cmake-build-debug/libNF_Tx_Core.so,
 *          ../Nf_Tx_Core/cmake-build-debug/libNF_Tx_Core.so, ./libNF_Tx_Core.so }
 *      csv-file   the transaction file (e.g. a BlockPit export)
 *      mode       0=CDC 1=Card 2=Default 3=Custom 4=Kraken 5=BlockPit (default: 5)
 *      out-file   series output file (default: wallet_series.txt)
 *      date       optional YYYY-MM-DD: instead of the full series, write a
 *                 per-symbol snapshot ("CUR;BALANCE;BONUS" with the last
 *                 series day at/before that date) — handy for "what was held
 *                 on <date>" questions.
 *      --extended also prints the core's wallet/transaction strings to the
 *                 console (the series file stays clean either way).
 *
 *    The CLI dumps the core's DAILY WALLET SERIES (one line per symbol per
 *    active day) for a transaction CSV, using the exact same core calls the
 *    app makes (`initWithData` -> `getDailyWalletSeries`). That makes it
 *    the ground truth for the balance side of the app's charts, without
 *    re-implementing trade semantics outside the core.
 *
 *    Output line format (the core's getDailyWalletSeries shape):
 *      CUR;YYYY-MM-DD;BALANCE;BONUS   (ascending per symbol)
 *
 * 2) NO arguments -> the interactive prompt: mode choice, the CSV fallback
 *    chain (repo root -> CWD -> fixtures/), price source, the per-symbol
 *    asset audit, the save/load round-trip exercise.
 *
 * The core prints its own log lines to the console while parsing; the
 * series file itself is clean.
 */
#include <dlfcn.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "LibLoader.h"
#include "StaticPrices.h"
#include "AssetValue.h"

namespace
{

    // The real export at the repo root wins (local dev reads the full
    // history); the committed reduced+anonymized fixtures under fixtures/
    // are the fallback for CI and fresh checkouts. `name` is the bare file
    // name.
    std::string resolveCsv(const std::string &name) {
        const std::vector<std::string> candidates = {
                "../" + name,               // repo root, run from the build dir
                "./" + name,               // run from the repo root
                "../fixtures/" + name,     // committed fixtures (CI / fresh checkout)
                "./fixtures/" + name};
        for (const auto &path : candidates) {
            if (std::filesystem::exists(path)) {
                std::cout << "Found CSV at: " << path << std::endl;
                return path;
            }
        }
        return "../" + name;   // keep the classic default for the error message
    }

    void printUsage(const std::string &program)
    {
        std::cerr << "Usage: " << program
                  << " [path/to/libNF_Tx_Core.so] <csv-file> [mode] [out-file] [date] [--extended]" << std::endl
                  << std::endl
                  << "  so-path    optional; auto-detected by default (../../Nf_Tx_Core/,\n                     ../Nf_Tx_Core/ and ./ build dirs, first existing wins)" << std::endl
                  << "  csv-file   the transaction file (e.g. a BlockPit export)" << std::endl
                  << "  mode       0=CDC 1=Card 2=Default 3=Custom 4=Kraken 5=BlockPit (default: 5)" << std::endl
                  << "  out-file   series output file (default: wallet_series.txt)" << std::endl
                  << "  date       optional YYYY-MM-DD: per-symbol snapshot \"CUR;BALANCE;BONUS\""
                       " with the last series day at/before that date" << std::endl
                  << "  --extended also prints the core's wallet/transaction strings to the console" << std::endl
                  << std::endl
                  << "With no arguments the interactive prompt starts instead." << std::endl;
    }

    bool findLibrary(const std::string &argument, std::string &path)
    {
        if (!argument.empty())
        {
            if (std::filesystem::exists(argument))
            {
                path = argument;
                return true;
            }
            std::cerr << "ERROR: library not found: " << argument << std::endl;
            return false;
        }
        const std::string candidates[] = {
            "../../Nf_Tx_Core/cmake-build-debug/libNF_Tx_Core.so",
            "../Nf_Tx_Core/cmake-build-debug/libNF_Tx_Core.so",
            "./libNF_Tx_Core.so",
        };
        for (const auto &candidate : candidates)
        {
            if (std::filesystem::exists(candidate))
            {
                path = candidate;
                return true;
            }
        }
        std::cerr << "ERROR: libNF_Tx_Core.so not found (pass its path as the first argument)" << std::endl;
        return false;
    }

    /**
     * The argv CLI: parses the CSV through the core's real call path and
     * writes the daily wallet series (or a date snapshot) to `outFile`.
     * The series FILE stays byte-identical in every mode combination;
     * `--extended` only adds console output.
     */
    int runWalletDump(const std::string &csvFile, int mode, const std::string &outFile,
                      const std::string &date, bool extended, const std::string &soArgument)
    {
        std::string soPath;
        if (!findLibrary(soArgument, soPath))
        {
            return 1;
        }

        std::ifstream file(csvFile);
        if (!file)
        {
            std::cerr << "ERROR: cannot open CSV: " << csvFile << std::endl;
            return 1;
        }
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line))
        {
            lines.push_back(line);
        }

        void *handle = dlopen(soPath.c_str(), RTLD_NOW);
        if (!handle)
        {
            std::cerr << "ERROR: dlopen failed: " << dlerror() << std::endl;
            return 1;
        }
        // 4-arg core signature: the 4th parameter is the parse budget in ms
        // (0 = no budget), added with the core's parse-timeout work.
        auto initWithData = reinterpret_cast<bool (*)(const std::vector<std::string> &, int, const std::string &, long long)>(
            dlsym(handle, "initWithData"));
        // cxx11-mangled: std::vector<std::string> getDailyWalletSeries()
        auto getDailyWalletSeries = reinterpret_cast<std::vector<std::string> (*)()>(
            dlsym(handle, "_Z20getDailyWalletSeriesB5cxx11v"));
        if (!initWithData || !getDailyWalletSeries)
        {
            std::cerr << "ERROR: core symbols missing (initWithData / getDailyWalletSeries). "
                         "Is this the current Nf_Tx_Core build?" << std::endl;
            return 1;
        }

        // NOTE: the core's FileLog always echoes to std::cout (no exported
        // switch), so the console shows its parsing log; the series FILE
        // below is clean.
        bool ok = initWithData(lines, mode, "wallet_dump_core.log", 5000);
        std::cout << "mode: " << mode << " (" << (mode == 5 ? "BlockPit" : "other")
                  << ")" << std::endl;
        std::cout << "init: " << (ok ? "OK" : "FAILED") << std::endl;
        if (!ok) return 1;

        const std::vector<std::string> series = getDailyWalletSeries();

        std::ofstream out(outFile);
        if (!out)
        {
            std::cerr << "ERROR: cannot open output file: " << outFile << std::endl;
            return 1;
        }

        int written = 0;
        if (date.empty())
        {
            for (const auto &s : series)
            {
                out << s << "\n";
                written++;
            }
        }
        else
        {
            // Snapshot at/before [date]: last series day per symbol.
            std::map<std::string, std::string> lastAtOrBefore;
            for (const auto &s : series)
            {
                std::vector<std::string> parts;
                std::istringstream ss(s);
                std::string part;
                while (std::getline(ss, part, ';')) parts.push_back(part);
                if (parts.size() < 3 || parts[1] > date) continue;
                lastAtOrBefore[parts[0]] = s;   // series is ascending, so last write wins
            }
            for (const auto &[symbol, row] : lastAtOrBefore)
            {
                out << row << "\n";
                written++;
            }
        }

        std::cout << "wrote " << written << " line(s) to " << outFile << std::endl;
        if (!date.empty()) std::cout << "(snapshot at/before " << date << ")" << std::endl;

        if (extended)
        {
            // Console-only listings on top of the clean series file. The
            // card getters mirror the app's is-card switch.
            auto getWalletsAsStrings = reinterpret_cast<std::vector<std::string> (*)()>(dlsym(handle, "getWalletsAsStrings"));
            auto getTransactionsAsStrings = reinterpret_cast<std::vector<std::string> (*)()>(dlsym(handle, "getTransactionsAsStrings"));
            if (mode == 1)
            {
                getWalletsAsStrings = reinterpret_cast<std::vector<std::string> (*)()>(dlsym(handle, "getCardWalletsAsStrings"));
                getTransactionsAsStrings = reinterpret_cast<std::vector<std::string> (*)()>(dlsym(handle, "getCardTransactionsAsStrings"));
            }
            if (getWalletsAsStrings && getTransactionsAsStrings)
            {
                std::cout << "Wallets: " << std::endl;
                for (const auto &item : getWalletsAsStrings()) std::cout << item << "\n";
                std::cout << "Transactions: " << std::endl;
                for (const auto &item : getTransactionsAsStrings()) std::cout << item << "\n";
            }
        }
        return 0;
    }

    /**
     * The interactive prompt (the classic tester loop). Verbatim
     * behaviour: mode choice with the fixture fallback chain, extended
     * output, price source (static / CoinGecko), the per-symbol asset
     * audit, and the save/load round-trip exercise.
     */
    int runInteractive() {

        bool isCrypto = true;
        bool isCard = false;

        bool extended = false;

        // Load the shared library
#ifdef _WIN32
        void* libHandle = loadLibrary(R"(..\..\Nf_Tx_Core\cmake-build-debug\libNF_Tx_Core.dll)");
#else
        void* libHandle = loadLibrary("../../Nf_Tx_Core/cmake-build-debug/libNF_Tx_Core.so");
#endif

        // 4-arg core signature: the 4th parameter is the parse budget in ms
        // (0 = no budget), added with the core's parse-timeout work.
        auto initWithData = loadSymbol<bool (*)(const std::vector<std::string>&, int, const std::string&, long long)>(libHandle, "initWithData");

        std::string filename;

        //ask user what mode to use
        std::cout << "Enter mode: " << std::endl;
        std::cout << "1. CDC" << std::endl;
        std::cout << "2. Card" << std::endl;
        std::cout << "3. Custom" << std::endl;
        std::cout << "4. Default" << std::endl;
        std::cout << "5. Kraken" << std::endl;
        std::cout << "6. BlockPit (Test CSV)" << std::endl;

        int mode;
        std::cin >> mode;

        switch (mode) {
            case 1:
                filename = resolveCsv("crypto_sample_transactions.csv");
                break;
            case 2:
                filename = resolveCsv("card_transactions_record_.csv");
                isCard = true;
                isCrypto = false;
                break;
            case 3:
                std::cout << "Enter filename: " << std::endl;
                std::cin >> filename;

                std::cout << "Mode: 1 for crypto, 2 for card" << std::endl;
                std::cout << "Enter mode: " << std::endl;
                std::cin >> mode;

                if (mode == 1) {
                    isCrypto = true;
                    isCard = false;
                }
                else if (mode == 2) {
                    isCrypto = false;
                    isCard = true;
                }
                else {
                    std::cout << "Invalid mode" << std::endl;
                    return 1;
                }

                break;
            case 4:
                filename = resolveCsv("crypto_sample_transactions.csv");
                mode = 1;   // Default = CDC
                break;
            case 5:
                // Kraken mode parses the Kraken trade format - the CRO-format
                // crypto_sample file was always 0/101 for it (wrong format), so
                // default to the synthetic Kraken fixture.
                filename = resolveCsv("kraken_trades.csv");
                mode = 5;   // Kraken enum value (will become 4 after mode--)
                break;
            case 6: {
                // Try multiple possible paths for the BlockPit test CSV
                // (real export first, committed fixtures as fallback)
                std::string possiblePaths[] = {
                    "blockpit_transactions.csv",
                    "../blockpit_transactions.csv",
                    "./blockpit_transactions.csv",
                    "../fixtures/blockpit_transactions.csv",
                    "./fixtures/blockpit_transactions.csv"
                };
                filename = "";
                for (const auto& path : possiblePaths) {
                    if (std::filesystem::exists(path)) {
                        filename = path;
                        std::cout << "Found CSV at: " << path << std::endl;
                        break;
                    }
                }
                if (filename.empty()) {
                    std::cout << "ERROR: blockpit_transactions.csv not found" << std::endl;
                    return 1;
                }
                mode = 6;   // BlockPit enum value (will become 5 after mode--)
                isCrypto = true;
                break;
            }
            default:
                std::cout << "Invalid mode" << std::endl;
                return 1;

        }

        //ask user if they want extended output
        std::cout << "Extended output (show all wallets/transactions)? (y/n)" << std::endl;
        std::string extendedInput;
        std::cin >> extendedInput;
        if (extendedInput == "y") extended = true;
        else if (extendedInput == "n") extended = false;
        else {
            std::cout << "Invalid input" << std::endl;
            return 1;
        }

        //ask user for price source
        bool useRealPrices = false;
        if (isCrypto) {
            std::cout << "\nPrice source: " << std::endl;
            std::cout << "1. Static hardcoded prices (fast, no network)" << std::endl;
            std::cout << "2. Real prices from CoinGecko API (requires internet, cached 24h)" << std::endl;
            std::cout << "Enter price source: " << std::endl;
            int priceSource;
            std::cin >> priceSource;
            if (priceSource == 1) {
                useRealPrices = false;
                std::cout << "Using static prices\n";
            } else if (priceSource == 2) {
                useRealPrices = true;
                std::cout << "Using real prices from CoinGecko (cached 24h)\n";
            } else {
                std::cout << "Invalid input, using static prices\n";
            }
        }

        // Print a summary header
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Loading: " << filename << std::endl;
        std::cout << "  Mode: " << (mode + 1) << " (" << (isCrypto ? "Crypto" : "Card") << ")" << std::endl;
        std::cout << "========================================\n" << std::endl;

        mode--;

        std::vector<std::string> data;

        std::ifstream file(filename);

        if (file) {
            std::string line;

            while (std::getline(file, line)) {
                data.push_back(line);
            }
        }

        //create log file
        std::ofstream logFile;
        logFile.open("log.txt");
        logFile.close();

        //get absolute path to log file
        std::string logFilePath = std::filesystem::current_path().string() + "/log.txt";
        std::string savePath = std::filesystem::current_path().string() + "/save/";


        auto isOk = initWithData(data, mode, logFilePath, 5000);

        std::cout << "isOk: " << isOk << std::endl;

        auto init = loadSymbol<bool (*)(std::string, std::string)>(libHandle, "init");

        auto isOk2 = init(logFilePath, savePath);

        std::cout << "isOk2: " << isOk2 << std::endl;

        auto getCurrencies = loadSymbol<std::vector<std::string> (*)()>(libHandle, "getCurrencies");

        auto currencies = getCurrencies();

        for (const auto& item: currencies) {
            if (extended) std::cout << item << std::endl;
        }

        //load all symbols

        auto setPrice = loadSymbol<void (*)(std::vector<double>)>(libHandle, "setPrice");
        auto getTotalMoneySpent = loadSymbol<double (*)()>(libHandle, isCrypto ? "getTotalMoneySpent" : "getTotalMoneySpentCard");
        auto getTotalValueOfAssets = loadSymbol<double (*)()>(libHandle, isCrypto ? "getTotalValueOfAssets" : "getTotalValueOfAssetsCard");
        auto getTotalBonus = loadSymbol<double (*)()>(libHandle, isCrypto ? "getTotalBonus" : "getTotalBonusCard");
        auto getValueOfAssets = loadSymbol<double (*)(int)>(libHandle, "getValueOfAssets");
        auto getBonus = loadSymbol<double (*)(int)>(libHandle, "getBonus");
        auto getMoneySpent = loadSymbol<double (*)(int)>(libHandle, "getMoneySpent");

        auto getWalletsAsStrings = loadSymbol<std::vector<std::string> (*)()>(libHandle, "getWalletsAsStrings");
        if (!isCrypto) getWalletsAsStrings = loadSymbol<std::vector<std::string> (*)()>(libHandle, "getCardWalletsAsStrings");

        auto getTransactionsAsStrings = loadSymbol<std::vector<std::string> (*)()>(libHandle, "getTransactionsAsStrings");
        if (!isCrypto) getTransactionsAsStrings = loadSymbol<std::vector<std::string> (*)()>(libHandle, "getCardTransactionsAsStrings");
        auto getWalletAsString = loadSymbol<std::string (*)(int)>(libHandle, "getWalletAsString");
        auto save = loadSymbol<void (*)(std::string)>(libHandle, "save");
        auto loadData = loadSymbol<void (*)(std::string)>(libHandle, "loadData");
        auto calculateBalances = loadSymbol<void (*)()>(libHandle, "calculateBalances");
        auto setWalletData = loadSymbol<void (*)(std::vector<std::string>)>(libHandle, "setWalletData");
        auto setCardWalletData = loadSymbol<void (*)(std::vector<std::string>)>(libHandle, "setCardWalletData");
        auto setTransactionData = loadSymbol<void (*)(std::vector<std::string>)>(libHandle, "setTransactionData");
        auto setCardTransactionData = loadSymbol<void (*)(std::vector<std::string>)>(libHandle, "setCardTransactionData");
        auto clearAll = loadSymbol<void (*)()>(libHandle, "clearAll");


        // Get prices (static or real from CoinGecko)
        std::cout << "Setting prices...\n";
        std::vector<double> prices;

        if (useRealPrices && isCrypto) {
            AssetValue assetValue;
            // Bulk fetch ALL prices at once - this is much faster and avoids rate limits
            std::cout << "Fetching prices from CoinGecko API (cached 24h to disk)...\n";
            assetValue.loadPrices(currencies);
            // Now get all prices (with caching)
            for (const auto &item: currencies) {
                prices.push_back(assetValue.getPrice(item));
            }
        } else {
            // Use static prices from StaticPrices.h
            std::cout << "Using static hardcoded prices\n";
            for (const auto &item: currencies) {
                prices.push_back(getPrice(item));
            }
        }

        setPrice(prices);

        if (isCrypto) {
            std::cout << "--- Asset Value Audit ---\n";
            double calculatedTotal = 0;
            for (size_t i = 0; i < currencies.size(); ++i) {
                std::vector<double> singlePrice(currencies.size(), 0.0);
                singlePrice[i] = prices[i];
                setPrice(singlePrice);
                double val = getTotalValueOfAssets();
                if (val > 0) {
                    std::cout << currencies[i] << ": " << val << " EUR (at price " << prices[i] << ")\n";
                    calculatedTotal += val;
                }
            }
            std::cout << "Audit Total: " << calculatedTotal << " EUR\n";
            // Reset to full prices to check total
            setPrice(prices);
            std::cout << "Core Total: " << getTotalValueOfAssets() << " EUR\n";
            std::cout << "------------------------\n";
        }

        if (isCrypto) {
            std::cout << "Currencies: " << std::endl;
            for (const auto &item: currencies) {
                std::cout << item << "\n";
            }
        }

        if (isCrypto) {
            std::cout << "Total money spent: " << getTotalMoneySpent() << std::endl;
            std::cout << "Total value of assets: " << getTotalValueOfAssets() << std::endl;
        }
        else
        {
            std::cout << "Total money spent: " << getTotalValueOfAssets() << std::endl;
        }
        if (isCrypto) std::cout << "Total bonus: " << getTotalBonus() << std::endl;

        if (isCrypto) std::cout << "Value of assets for wallet 2: " << getValueOfAssets(2) << std::endl;
        if (isCrypto) std::cout << "Bonus for wallet 2: " << getBonus(2) << std::endl;
        std::cout << "Money spent for wallet 2: " << getMoneySpent(2) << std::endl;

        std::cout << "Wallet 2: " << getWalletAsString(2) << std::endl;

        std::cout << "Wallets: " << std::endl;
        for (const auto& item: getWalletsAsStrings()) {
            if (extended) std::cout << item << "\n";
        }
        std::cout << "Transactions: " << std::endl;
        for (const auto& item: getTransactionsAsStrings()) {
            if (extended) std::cout << item << "\n";
        }

        if (extended) std::endl(std::cout);

        std::cout << "\n\nSaving data..." << std::endl;

        std::cout << "Save path: " << savePath << std::endl;

        //make sure save directory exists
        std::filesystem::create_directory(savePath);

        //measure time
        auto start = std::chrono::high_resolution_clock::now();

        save(savePath);

        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Done" << std::endl;

        std::cout << "Saving took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;


        std::cout << "Loading data..." << std::endl;
        start = std::chrono::high_resolution_clock::now();
        loadData(savePath);
        end = std::chrono::high_resolution_clock::now();
        std::cout << "Done" << std::endl;
        std::cout << "Loading took: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;

        std::cout << "Calculating balances..." << std::endl;
        calculateBalances();
        std::cout << "Done" << std::endl;

        if (isCrypto) {
            std::cout << "Total money spent: " << getTotalMoneySpent() << std::endl;
            std::cout << "Total value of assets: " << getTotalValueOfAssets() << std::endl;
            std::cout << "Total bonus: " << getTotalBonus() << std::endl;
        }
        else
        {
            std::cout << "Total money spent: " << getTotalValueOfAssets() << std::endl;
        }

        std::cout << "\n\nTesting setWalletData, setCardWalletData, setTransactionData, setCardTransactionData" << std::endl;

        auto wallets = getWalletsAsStrings();
        auto cardWallets = getWalletsAsStrings();
        auto transactions = getTransactionsAsStrings();
        auto cardTransactions = getTransactionsAsStrings();

        clearAll();

        if (isCrypto) {
            setWalletData(wallets);
            setTransactionData(transactions);
        }

        if (isCard) {
            setCardWalletData(cardWallets);
            setCardTransactionData(cardTransactions);
        }

        setPrice(prices);

        if (isCrypto) {
            std::cout << "Total money spent: " << getTotalMoneySpent() << std::endl;
            std::cout << "Total value of assets: " << getTotalValueOfAssets() << std::endl;
        }
        else
        {
            std::cout << "Total money spent: " << getTotalValueOfAssets() << std::endl;
        }
        if (isCrypto) std::cout << "Total bonus: " << getTotalBonus() << std::endl;

        if (isCrypto) std::cout << "Value of assets for wallet 2: " << getValueOfAssets(2) << std::endl;
        if (isCrypto) std::cout << "Bonus for wallet 2: " << getBonus(2) << std::endl;
        std::cout << "Money spent for wallet 2: " << getMoneySpent(2) << std::endl;

        std::cout << "Wallet 2: " << getWalletAsString(2) << std::endl;

        //print all wallets
        std::cout << "Wallets: " << std::endl;
        for (const auto& item: wallets) {
            if (extended) std::cout << item << "\n";
        }



        return 0;
    }

} // namespace

int main(int argc, char** argv)
{
    // Scan the arguments: the help/extended FLAGS are pulled out of the
    // positional stream, so the [so] <csv> [mode] [out] [date] heuristic
    // below (and the no-arg interactive fallback) is unchanged.
    bool help = false;
    bool extended = false;
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++)
    {
        const std::string argument(argv[i]);
        if (argument == "--help" || argument == "-h") { help = true; continue; }
        if (argument == "--extended") { extended = true; continue; }
        args.push_back(argument);
    }

    if (help)
    {
        printUsage(argv[0]);
        return 0;
    }
    if (args.empty())
    {
        return runInteractive();
    }

    // [so-path] csv [mode] [out] [date]: the library path (if present) is
    // the first argument that exists on disk with a lib extension; the CSV
    // is the first non-matching argument.
    std::string soArgument;
    std::vector<std::string> rest;
    for (const auto &argument : args)
    {
        if (soArgument.empty() && std::filesystem::exists(argument) &&
            std::filesystem::is_regular_file(argument) &&
            (argument.find(".so") != std::string::npos || argument.find(".dll") != std::string::npos ||
             argument.find(".dylib") != std::string::npos))
        {
            soArgument = argument;
        }
        else
        {
            rest.push_back(argument);
        }
    }
    if (rest.empty())
    {
        printUsage(argv[0]);
        return 1;
    }

    const std::string csvFile = rest[0];
    int mode = 5;                     // BlockPit (the app's main import path)
    std::string outFile = "wallet_series.txt";
    std::string date;
    if (rest.size() > 1) mode = std::atoi(rest[1].c_str());
    if (rest.size() > 2) outFile = rest[2];
    if (rest.size() > 3) date = rest[3];

    return runWalletDump(csvFile, mode, outFile, date, extended, soArgument);
}
