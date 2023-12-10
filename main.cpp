#include <iostream>
#include <dlfcn.h> // Dynamic Loading Library
#include <vector>
#include <fstream>
#include <sstream>
#include "StaticPrices.h"

int main() {

    void* libHandle = dlopen("../../NF-Tx-Core/cmake-build-debug/libNF_Tx_Core.so", RTLD_LAZY);

    if (!libHandle) {
        std::cout << "Error loading library: " << dlerror() << std::endl;
        return 1;
    }

    typedef bool (*initWithData_t)(std::vector<std::string> data, uint mode);
    auto initWithData = (initWithData_t) dlsym(libHandle, "initWithData");

    if (!initWithData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    const std::string& filename = "../crypto_sample_transactions.csv";
    std::vector<std::string> data;

    std::ifstream file(filename);

    if (file) {
        std::string line;

        while (std::getline(file, line)) {
            data.push_back(line);
        }
    }

    auto isOk = initWithData(data, 0);

    std::cout << "isOk: " << isOk << std::endl;

    typedef bool (*init_t)();
    auto init = (init_t) dlsym(libHandle, "init");

    if (!init) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    auto isOk2 = init();

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

    if (!getTotalMoneySpent) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getTotalValueOfAssets_t)();
    auto getTotalValueOfAssets = (getTotalValueOfAssets_t) dlsym(libHandle, "getTotalValueOfAssets");

    if (!getTotalValueOfAssets) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef double (*getTotalBonus_t)();
    auto getTotalBonus = (getTotalBonus_t) dlsym(libHandle, "getTotalBonus");

    if (!getTotalBonus) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    std::vector<double> prices;
    for (const auto &item: currencies) {
        prices.push_back(getPrice(item));
    }

    setPrice(prices);
    std::cout << "Total money spent: " << getTotalMoneySpent() << std::endl;
    std::cout << "Total value of assets: " << getTotalValueOfAssets() << std::endl;
    std::cout << "Total bonus: " << getTotalBonus() << std::endl;

    return 0;
}


