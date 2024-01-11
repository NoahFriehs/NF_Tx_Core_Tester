//
// Created by nfriehs on 1/4/24.
//

#ifndef NF_TX_CORE_TESTER_LIBLOADER_H
#define NF_TX_CORE_TESTER_LIBLOADER_H

#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Function to load the shared library
void* loadLibrary(const char* libraryPath) {
#ifdef _WIN32
    HMODULE libHandle = LoadLibraryA(libraryPath);
#else
    void* libHandle = dlopen(libraryPath, RTLD_LAZY);
#endif
    if (!libHandle) {
#ifdef _WIN32
        std::cout << "Error loading library: " << GetLastError() << std::endl;
#else
        std::cout << "Error loading library: " << dlerror() << std::endl;
#endif
        std::exit(1);
    }

    return libHandle;
}

// Function template to load a symbol from the library
template<typename T>
T loadSymbol(void* libHandle, const char* symbolName) {
#ifdef _WIN32
    T symbol = reinterpret_cast<T>(GetProcAddress(static_cast<HMODULE>(libHandle), symbolName));
#else
    T symbol = reinterpret_cast<T>(dlsym(libHandle, symbolName));
#endif

    if (!symbol) {
#ifdef _WIN32
        std::cout << "Error loading symbol: " << GetLastError() << std::endl;
        FreeLibrary(static_cast<HMODULE>(libHandle));
#else
        std::cout << "Error loading symbol: " << dlerror() << std::endl;
        dlclose(libHandle);
#endif
        std::exit(1);
    }

    return symbol;
}


#endif //NF_TX_CORE_TESTER_LIBLOADER_H
