#ifndef NF_TX_CORE_TESTER_COINGECKO_PRICE_PROVIDER_H
#define NF_TX_CORE_TESTER_COINGECKO_PRICE_PROVIDER_H

#include "PriceProvider.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <ctime>

/**
 * Implementation of PriceProvider that fetches prices from CoinGecko API.
 * Mirrors CryptoPricesCryptoCompare.kt from Android.
 *
 * Uses the free CoinGecko API: https://docs.coingecko.com/v3.0/reference/introduction
 * No API key required for basic usage (rate limited to 10-30 calls/min).
 */
class CoinGeckoPriceProvider : public PriceProvider {
public:
    CoinGeckoPriceProvider();

    /**
     * Fetches the price of a single symbol from CoinGecko.
     * @param symbol The crypto symbol (e.g. "BTC", "ETH", "SOL")
     * @return Price in EUR, or 0.0 on failure.
     */
    double getPrice(const std::string &symbol) override;

    /**
     * Fetches multiple prices in bulk using single API call.
     * @param symbols List of symbols to fetch.
     * @return Map of symbol -> price in EUR.
     */
    std::map<std::string, double> getPricesBulk(const std::vector<std::string> &symbols);
    
    /**
     * Checks if we are currently rate-limited (for user prompt).
     */
    bool isRateLimited() const;

    /**
     * Resolves a symbol to its CoinGecko canonical ID.
     * Uses map, then cache, then API search.
     */
    std::string resolveSymbolToId(const std::string &symbol);

private:
    static bool isSafeSymbol(const std::string &symbol);

    /**
     * Converts a symbol to CoinGecko's internal currency ID format.
     * e.g., "BTC" -> "bitcoin", "ETH" -> "ethereum"
     */
    static std::string symbolToCoinGeckoId(const std::string &symbol);


    /**
     * Parses a JSON response and extracts the EUR price.
     */
    static double parsePriceFromJson(const std::string &json);

    /** Simple in-memory rate limiter to avoid API throttling */
    mutable std::mutex mutex_;
    struct CallInfo {
        std::string symbol;
        time_t timestamp;
    };
    std::vector<CallInfo> recentCalls_;
    mutable std::mutex rateLimitMutex_;
    bool rateLimited_ = false;
    time_t rateLimitUntil_ = 0;
    
    constexpr static size_t MAX_CALLS_PER_MINUTE = 15;
    constexpr static time_t COOLDOWN_SECONDS = 4;
    
    /**
     * Check if we should back off due to rate limiting.
     */
    void checkRateLimit();
    
    /**
     * Set rate limit state.
     */
    void setRateLimited(time_t until);

    std::map<std::string, std::string> symbolIdCache_;
    const std::string CACHE_FILE_IDS = "symbol_id_cache.json";
    int consecutiveRateLimits_ = 0;
    
    void loadSymbolCache();
    void saveSymbolCache();
};
#endif //NF_TX_CORE_TESTER_COINGECKO_PRICE_PROVIDER_H
