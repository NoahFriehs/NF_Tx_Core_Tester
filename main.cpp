#include <iostream>
#include <dlfcn.h> // Dynamic Loading Library
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "StaticPrices.h"

int main() {

    bool isCrypto = true;
    bool isCard = false;

    void* libHandle = dlopen("../../NF-Tx-Core/cmake-build-debug/libNF_Tx_Core.so", RTLD_LAZY);

    if (!libHandle) {
        std::cout << "Error loading library: " << dlerror() << std::endl;
        return 1;
    }

    typedef bool (*initWithData_t)(std::vector<std::string> data, uint mode,  std::string logFilePath);
    auto initWithData = (initWithData_t) dlsym(libHandle, "initWithData");

    if (!initWithData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    std::string filename;

    //ask user what mode to use
    std::cout << "Enter mode: " << std::endl;
    std::cout << "1. CDC" << std::endl;
    std::cout << "2. Card" << std::endl;
    std::cout << "3. Custom" << std::endl;
    std::cout << "4. Default" << std::endl;

    uint mode;
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
            break;
        case 4:
            filename = "../crypto_sample_transactions.csv";
            break;
        default:
            std::cout << "Invalid mode" << std::endl;
            return 1;

    }

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

    typedef bool (*init_t)(std::string logFilePath, std::string loadDirPath);
    auto init = (init_t) dlsym(libHandle, "init");

    if (!init) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    auto isOk2 = init(logFilePath, savePath);

    std::cout << "isOk2: " << isOk2 << std::endl;

    typedef std::vector<std::string> (*getCurrencies_t)();
    auto getCurrencies = (getCurrencies_t) dlsym(libHandle, "getCurrencies");

    if (!getCurrencies) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    auto currencies = getCurrencies();

    for (const auto& item: currencies) {
        //std::cout << item << std::endl;
    }

    typedef void (*setPrice_t)(std::vector<double> prices);
    auto setPrice = (setPrice_t) dlsym(libHandle, "setPrice");

    if (!setPrice) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getTotalMoneySpent_t)();
    auto getTotalMoneySpent = (getTotalMoneySpent_t) dlsym(libHandle, "getTotalMoneySpent");
    if (!isCrypto) getTotalMoneySpent = (getTotalMoneySpent_t) dlsym(libHandle, "getTotalMoneySpentCard");

    if (!getTotalMoneySpent) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getTotalValueOfAssets_t)();
    auto getTotalValueOfAssets = (getTotalValueOfAssets_t) dlsym(libHandle, "getTotalValueOfAssets");
    if (!isCrypto) getTotalValueOfAssets = (getTotalValueOfAssets_t) dlsym(libHandle, "getTotalValueOfAssetsCard");

    if (!getTotalValueOfAssets) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getTotalBonus_t)();
    auto getTotalBonus = (getTotalBonus_t) dlsym(libHandle, "getTotalBonus");
    if (!isCrypto) getTotalBonus = (getTotalBonus_t) dlsym(libHandle, "getTotalBonusCard");

    if (!getTotalBonus) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getValueOfAssets_t)(int walletId);
    auto getValueOfAssets = (getValueOfAssets_t) dlsym(libHandle, "getValueOfAssets");

    if (!getValueOfAssets) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getBonus_t)(int walletId);
    auto getBonus = (getBonus_t) dlsym(libHandle, "getBonus");

    if (!getBonus) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getMoneySpent_t)(int walletId);
    auto getMoneySpent = (getMoneySpent_t) dlsym(libHandle, "getMoneySpent");

    if (!getMoneySpent) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef std::vector<std::string> (*getWalletsAsStrings_t)();
    auto getWalletsAsStrings = (getWalletsAsStrings_t) dlsym(libHandle, "getWalletsAsStrings");
    if (!isCrypto) getWalletsAsStrings = (getWalletsAsStrings_t) dlsym(libHandle, "getCardWalletsAsStrings");

    if (!getWalletsAsStrings) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef std::vector<std::string> (*getTransactionsAsStrings_t)();
    auto getTransactionsAsStrings = (getTransactionsAsStrings_t) dlsym(libHandle, "getTransactionsAsStrings");
    if (!isCrypto) getTransactionsAsStrings = (getTransactionsAsStrings_t) dlsym(libHandle, "getCardTransactionsAsStrings");

    if (!getTransactionsAsStrings) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef std::string (*getWalletAsString_t)(int walletId);
    auto getWalletAsString = (getWalletAsString_t) dlsym(libHandle, "getWalletAsString");

    if (!getWalletAsString) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef void (*save_t)(std::string filePath);
    auto save = (save_t) dlsym(libHandle, "save");

    if (!save) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef void (*loadData_t)(std::string filePath);
    auto loadData = (loadData_t) dlsym(libHandle, "loadData");

    if (!loadData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef void (*calculateBalances_t)();
    auto calculateBalances = (calculateBalances_t) dlsym(libHandle, "calculateBalances");

    if (!calculateBalances) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef void (*setWalletData_t)(std::vector<std::string> data);
    auto setWalletData = (setWalletData_t) dlsym(libHandle, "setWalletData");

    if (!setWalletData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef void (*setCardWalletData_t)(std::vector<std::string> data);
    auto setCardWalletData = (setCardWalletData_t) dlsym(libHandle, "setCardWalletData");

    if (!setCardWalletData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef void (*setTransactionData_t)(std::vector<std::string> data);
    auto setTransactionData = (setTransactionData_t) dlsym(libHandle, "setTransactionData");

    if (!setTransactionData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef void (*setCardTransactionData_t)(std::vector<std::string> data);
    auto setCardTransactionData = (setCardTransactionData_t) dlsym(libHandle, "setCardTransactionData");

    if (!setCardTransactionData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }



    typedef void (*clearAll_t)();
    auto clearAll = (clearAll_t) dlsym(libHandle, "clearAll");

    if (!clearAll) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    std::vector<double> prices;
    for (const auto &item: currencies) {
        prices.push_back(getPrice(item));
    }

    setPrice(prices);

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
        //std::cout << item << "\n";
    }
    std::cout << "Transactions: " << std::endl;
    for (const auto& item: getTransactionsAsStrings()) {
        //std::cout << item << "\n";
    }

    //std::endl(std::cout);

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
        //std::cout << item << "\n";
    }



    return 0;
}


