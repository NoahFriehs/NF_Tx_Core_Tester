//
// Created by nfriehs on 1/4/24.
//

#ifndef NF_TX_CORE_TESTER_LIBLOADER_H
#define NF_TX_CORE_TESTER_LIBLOADER_H

#include <iostream>
#include <dlfcn.h>
#include <vector>

// Function to load the shared library
void* loadLibrary(const char* libraryPath) {
    void* libHandle = dlopen(libraryPath, RTLD_LAZY);

    if (!libHandle) {
        std::cout << "Error loading library: " << dlerror() << std::endl;
        std::exit(1);
    }

    return libHandle;
}

// Function template to load a symbol from the library
template<typename T>
T loadSymbol(void* libHandle, const char* symbolName) {
    T symbol = reinterpret_cast<T>(dlsym(libHandle, symbolName));

    if (!symbol) {
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
        std::exit(1);
    }

    return symbol;
}


#endif //NF_TX_CORE_TESTER_LIBLOADER_H
