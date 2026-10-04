#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include "LibLoader.h"
#include "StaticPrices.h"
#include "AssetValue.h"

int main() {

    bool isCrypto = true;
    bool isCard = false;

    bool extended = false;

    // Load the shared library
#ifdef _WIN32
    void* libHandle = loadLibrary(R"(..\..\NF_Tx_Core\cmake-build-debug\libNF_Tx_Core.dll)");
#else
    void* libHandle = loadLibrary("../../NF-Tx-Core/cmake-build-debug/libNF_Tx_Core.so");
#endif

    auto initWithData = loadSymbol<bool (*)(std::vector<std::string>, int, std::string)>(libHandle, "initWithData");

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
            filename = "../crypto_sample_transactions.csv";
            break;
        case 2:
            filename = "../card_transactions_record_.csv";
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
            filename = "../crypto_sample_transactions.csv";
            mode = 1;   // Default = CDC
            break;
        case 5:
            filename = "../crypto_sample_transactions.csv";  // Use the same Kraken test file
            mode = 5;   // Kraken enum value (will become 4 after mode--)
            break;
        case 6: {
            // Try multiple possible paths for the BlockPit test CSV
            std::string possiblePaths[] = {
                "blockpit_transactions.csv",
                "../blockpit_transactions.csv",
                "./blockpit_transactions.csv"
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


    auto isOk = initWithData(data, mode, logFilePath);

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


