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

# ---------------------------------------------------------------------------
# CTest: run the committed core fixtures through the argv CLI in no-price
# modes (offline — the CI network path is a 429 minefield, so price modes
# stay OUT of ctest). These use the CORE's fixtures (not the tester's own
# reduced copies) on purpose: the point is running the core's data through
# the app's exact call path (initWithData -> getDailyWalletSeries).
# The wallet assertions themselves live in the core's own suite.
#
# NF_TX_CORE_SO: which core library the tests load. Default = the core's
# local debug build (classic local dev); CI overrides it with the plain
# Release library from the core CI's `build-shared` step.
# ---------------------------------------------------------------------------
set(NF_TX_CORE_SO "${NF_TX_CORE_DIR}/cmake-build-debug/libNF_Tx_Core.so"
    CACHE FILEPATH "Core shared library the ctest entries load")

enable_testing()

# 'file|mode|pass-regex' triples (mode = the parser family each fixture
# was built for). Two delimiters at work: in CMake a
# semicolon is the LIST separator even inside quoted items ("file.csv;0"
# would split into two entries), and ctest's PASS_REGULAR_EXPRESSION is
# OR-semantics (any one match passes) - so each test gets exactly ONE
# expect line. Crash/failure paths are already fatal via the exit code
# (dlopen/dlsym/CSV errors and a failed parse all return 1).
#
# The card fixture legitimately yields an EMPTY series (card = spending
# only, no held assets - the app's card scope shows 0 EUR assets), so it
# asserts the parse, not a non-empty series. The money fixtures must
# produce a non-empty one (that is where #44-class regressions - parse
# OK but wrong/empty series data - surface).
set(NF_TX_CORE_TEST_FIXTURES
    "cdc_crypto.csv|0|wrote [1-9][0-9]* line"
    "cdc_card.csv|1|init: OK"
    "kraken_trades.csv|4|wrote [1-9][0-9]* line"
    "blockpit_export.csv|5|wrote [1-9][0-9]* line")

foreach(entry IN LISTS NF_TX_CORE_TEST_FIXTURES)
    string(REPLACE "|" ";" triple "${entry}")
    list(GET triple 0 fixture)
    list(GET triple 1 mode)
    list(GET triple 2 pass_regex)
    string(REGEX REPLACE "\\.csv$" "" base "${fixture}")
    add_test(NAME "fixture_${base}_mode${mode}"
             COMMAND NF_Tx_Core_Tester "${NF_TX_CORE_SO}"
                     "${NF_TX_CORE_DIR}/Tests/fixtures/${fixture}"
                     "${mode}" "${CMAKE_CURRENT_BINARY_DIR}/ctest_${base}_series.txt")
    set_tests_properties("fixture_${base}_mode${mode}" PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        PASS_REGULAR_EXPRESSION "${pass_regex}")
endforeach()
