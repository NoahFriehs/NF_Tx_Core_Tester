#ifndef NF_TX_CORE_TESTER_PRICE_CACHE_H
#define NF_TX_CORE_TESTER_PRICE_CACHE_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <vector>
#include <optional>

/**
 * File-backed Price Cache with 24-hour TTL.
 * Mirrors Android's PriceCache but persists to disk to avoid rate limiting.
 * 
 * Prices are saved to a JSON file between runs.
 * Only fetches if cached price is older than 24 hours.
 */
class PriceCache {
public:
    static constexpr const char* CACHE_FILE = "prices.json";
    static constexpr std::chrono::hours TTL = std::chrono::hours(24);
    
    PriceCache();
    ~PriceCache();
    
    /**
     * Loads prices from file cache.
     */
    void loadFromFile();
    
    /**
     * Saves all prices to file.
     */
    void saveToFile() const;
    
    /**
     * Gets a price from cache. Returns -1.0 if not found or expired.
     * @param symbol The symbol to get.
     * @return Price value or -1.0.
     */
    double get(const std::string &symbol) const;
    
    /**
     * Sets a price in cache.
     * @param symbol The symbol.
     * @param price The price value.
     */
    void set(const std::string &symbol, double price);
    
    /**
     * Tests if a symbol is cached and valid (not expired).
     * @param symbol The symbol to check.
     * @return True if cached and valid.
     */
    bool isValid(const std::string &symbol) const;
    
    /**
     * Returns all non-expired cached symbols.
     */
    std::vector<std::string> getSymbols() const;
    
    /**
     * Returns count of valid (non-expired) cached entries.
     */
    size_t size() const;
    
    /**
     * Clears all cached prices.
     */
    void clear();
    
private:
    struct CacheEntry {
        double price;
        int64_t timestamp;  // Unix timestamp in seconds
        
        bool isExpired() const {
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            return std::difftime(now, timestamp) > 86400;  // 24 hours
        }
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CacheEntry> cache_;
    
    /**
     * Simple JSON parsing for price cache format:
     * {"BTC": {"price": 65000.0, "time": 1709577600}, ...}
     */
    static std::unordered_map<std::string, CacheEntry> parseJson(const std::string &json);
    
    /**
     * Simple JSON serialization.
     */
    static std::string serializeJson(const std::unordered_map<std::string, CacheEntry> &data);
};

#endif //NF_TX_CORE_TESTER_PRICE_CACHE_H
