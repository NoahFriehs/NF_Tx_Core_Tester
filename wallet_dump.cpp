/**
 * NF_Tx_Core_WalletDump
 *
 * Dumps the core's DAILY WALLET SERIES (one line per symbol per active
 * day) for a transaction CSV, using the exact same core calls the app
 * makes (`initWithData` -> `getDailyWalletSeries`). That makes it the
 * ground truth for the balance side of the app's charts, without
 * re-implementing trade semantics outside the core.
 *
 * Usage:
 *   NF_Tx_Core_WalletDump [path/to/libNF_Tx_Core.so] <csv-file> [mode] [out-file] [date]
 *
 *   so-path    optional; default: ../../Nf_Tx_Core/cmake-build-debug/libNF_Tx_Core.so
 *   csv-file   the transaction file (e.g. a BlockPit export)
 *   mode       0=CDC 1=Card 2=Default 3=Custom 4=Kraken 5=BlockPit (default: 5)
 *   out-file   series output file (default: wallet_series.txt)
 *   date       optional YYYY-MM-DD: instead of the full series, write a
 *              per-symbol snapshot ("CUR;BALANCE;BONUS" with the last
 *              series day at/before that date) — handy for "what was held
 *              on <date>" questions.
 *
 * Output line format (the core's getDailyWalletSeries shape):
 *   CUR;YYYY-MM-DD;BALANCE;BONUS   (ascending per symbol)
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

namespace
{

    bool findLibrary(const std::string& argument, std::string& path)
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
        for (const auto& candidate : candidates)
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

}

int main(int argc, char** argv)
{
    // argv: [prog] [so-path] csv [mode] [out] [date]
    int arg = 1;
    std::string soArgument;
    // Heuristic: the library path (if present) is the first argument that
    // exists on disk; the CSV is the first non-matching argument.
    std::vector<std::string> rest;
    for (int i = 1; i < argc; i++)
    {
        const std::string argument(argv[i]);
        if (soArgument.empty() && std::filesystem::exists(argument) &&
            std::filesystem::is_regular_file(argument) &&
            (argument.find(".so") != std::string::npos || argument.find(".dll") != std::string::npos ||
             argument.find(".dylib") != std::string::npos))
        {
            soArgument = argv[i];
        }
        else
        {
            rest.push_back(argv[i]);
        }
    }
    if (rest.empty())
    {
        std::cerr << "Usage: " << argv[0] << " [path/to/libNF_Tx_Core.so] <csv-file> [mode] [out-file] [date]" << std::endl;
        return 1;
    }

    std::string soPath;
    if (!findLibrary(soArgument, soPath))
    {
        return 1;
    }

    const std::string csvFile = rest[0];
    int mode = 5;                     // BlockPit (the app's main import path)
    std::string outFile = "wallet_series.txt";
    std::string date;
    if (rest.size() > 1) mode = std::atoi(rest[1].c_str());
    if (rest.size() > 2) outFile = rest[2];
    if (rest.size() > 3) date = rest[3];

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

    void* handle = dlopen(soPath.c_str(), RTLD_NOW);
    if (!handle)
    {
        std::cerr << "ERROR: dlopen failed: " << dlerror() << std::endl;
        return 1;
    }
    auto initWithData = reinterpret_cast<bool (*)(const std::vector<std::string>&, int, const std::string&, long long)>(
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
    // switch), so the console shows its parsing log; the series FILE below
    // is clean.
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
        for (const auto& s : series)
        {
            out << s << "\n";
            written++;
        }
    }
    else
    {
        // Snapshot at/before [date]: last series day per symbol.
        std::map<std::string, std::string> lastAtOrBefore;
        for (const auto& s : series)
        {
            std::vector<std::string> parts;
            std::istringstream ss(s);
            std::string part;
            while (std::getline(ss, part, ';')) parts.push_back(part);
            if (parts.size() < 3 || parts[1] > date) continue;
            lastAtOrBefore[parts[0]] = s;   // series is ascending, so last write wins
        }
        for (const auto& [symbol, row] : lastAtOrBefore)
        {
            out << row << "\n";
            written++;
        }
    }

    std::cout << "wrote " << written << " line(s) to " << outFile << std::endl;
    if (!date.empty()) std::cout << "(snapshot at/before " << date << ")" << std::endl;
    return 0;
}
