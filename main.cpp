#include <iostream>
#include <dlfcn.h> // Dynamic Loading Library
#include <vector>
#include <fstream>
#include <sstream>

int main() {

    void* libHandle = dlopen("../../NF-Tx-Core/cmake-build-debug/libNF_Tx_Core.so", RTLD_LAZY);

    if (!libHandle) {
        std::cout << "Error loading library: " << dlerror() << std::endl;
        return 1;
    }

    typedef void (*hello_t)();
    auto hello = (hello_t) dlsym(libHandle, "hello");

    if (!hello) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    typedef bool (*initWithData_t)(std::vector<std::string> data, uint mode);
    auto initWithData = (initWithData_t) dlsym(libHandle, "initWithData");

    if (!initWithData) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        return 1;
    }

    hello();

    const std::string& filename = "filename.csv";
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

    return 0;
}
