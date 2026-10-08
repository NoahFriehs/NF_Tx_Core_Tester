cmake_minimum_required(VERSION 3.22.1)
project(NF_Tx_Core_Tester)

set(CMAKE_CXX_STANDARD 20)

add_executable(NF_Tx_Core_Tester main.cpp
        LibLoader.h
        StaticPrices.h
        PriceProvider.h
        CoinGeckoPriceProvider.h
        CoinGeckoPriceProvider.cpp
        PriceCache.h
        PriceCache.cpp
        AssetValue.h
        AssetValue.cpp)

# Link required libraries
if(WIN32)
    target_link_libraries(NF_Tx_Core_Tester PRIVATE ntdll)
else()
    target_link_libraries(NF_Tx_Core_Tester PRIVATE dl pthread)
endif()

# Wallet series dumper (CLI tool): NF_Tx_Core_WalletDump [so] <csv> [mode] [out] [date]
add_executable(NF_Tx_Core_WalletDump wallet_dump.cpp)
if(WIN32)
    target_link_libraries(NF_Tx_Core_WalletDump PRIVATE ntdll)
else()
    target_link_libraries(NF_Tx_Core_WalletDump PRIVATE dl pthread)
endif()
