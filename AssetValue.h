#ifndef NF_TX_CORE_TESTER_ASSET_VALUE_H
#define NF_TX_CORE_TESTER_ASSET_VALUE_H

#include "CoinGeckoPriceProvider.h"
#include "PriceCache.h"
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <atomic>

/**
 * Manages asset prices with caching and network fetching.
 * Mirrors AssetValue.kt from Android.
 * 
 * Usage:
 *   AssetValue assetValue;
 *   assetValue.loadPrices(currencies);  // Fetches prices in background
 *   double btcPrice = assetValue.getPrice("BTC");  // Uses cache or fetches from network
 */
class AssetValue {
public:
    AssetValue();
    ~AssetValue();

    /**
     * Gets the price of a symbol. Uses cache first, then fetches from network.
     * @param symbol The crypto symbol.
     * @return Price in EUR, or 0.0 on failure.
     */
    double getPrice(const std::string &symbol);

    /**
     * Loads prices for all symbols from the network.
     * Runs synchronously (waits for all prices to be fetched).
     * @param symbols List of symbols to fetch.
     * @return True if all prices were fetched successfully.
     */
    bool loadPrices(const std::vector<std::string> &symbols);

    /**
     * Refreshes all cached prices from the network.
     * @return True if refresh succeeded.
     */
    bool refreshCache();

    /**
     * Checks if price fetching is available (has network).
     */
    bool isConnected() const;

    /**
     * Gets the number of prices currently cached.
     */
    size_t cacheSize() const;
    
    /**
     * Loads cache from file.
     */
    void loadCache();
    
    /**
     * Saves current prices to file.
     */
    void saveCache() const;

private:
    std::unique_ptr<CoinGeckoPriceProvider> priceProvider_;
    PriceCache cache_;
    std::atomic<bool> isRunning_;
    mutable std::mutex mutex_;
};

#endif //NF_TX_CORE_TESTER_ASSET_VALUE_H
