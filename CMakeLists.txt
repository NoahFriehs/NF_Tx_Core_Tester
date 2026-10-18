cmake_minimum_required(VERSION 3.22.1)
project(NF_Tx_Core_Tester)

set(CMAKE_CXX_STANDARD 20)

# Canonical core checkout (sibling dir). Cache variable because the local
# folder name may differ from the committed one (main.cpp's runtime lib path
# is overridden independently).
set(NF_TX_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Nf_Tx_Core"
    CACHE PATH "Path to the canonical Nf_Tx_Core checkout (headers)")

add_executable(NF_Tx_Core_Tester main.cpp
        LibLoader.h
        PriceProvider.h
        CoinGeckoPriceProvider.h
        CoinGeckoPriceProvider.cpp
        PriceCache.h
        PriceCache.cpp
        AssetValue.h
        AssetValue.cpp)

# StaticPrices.h is single-sourced from the core repo (no local copy).
target_include_directories(NF_Tx_Core_Tester PRIVATE "${NF_TX_CORE_DIR}/Price")

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
