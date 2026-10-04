#include "AssetValue.h"
#include <iostream>

AssetValue::AssetValue() : priceProvider_(std::make_unique<CoinGeckoPriceProvider>()), isRunning_(false) {
    isRunning_.store(true);
    loadCache();
}

AssetValue::~AssetValue() {
    saveCache();
    isRunning_.store(false);
}

double AssetValue::getPrice(const std::string &symbol) {
    if (symbol == "EUR") return 1.0;

    // Check cache first
    double cached = cache_.get(symbol);
    if (cached != -1.0) return cached;

    // Always use bulk fetch for reliability
    double price = 0.0;
    if (isRunning_.load()) {
        auto prices = priceProvider_->getPricesBulk({symbol});
        if (!prices.empty()) {
            price = prices[symbol];
            if (price > 0) {
                cache_.set(symbol, price);
            }
        }
    }

    return price;
}

bool AssetValue::loadPrices(const std::vector<std::string> &symbols) {
    if (symbols.empty()) return true;

    // Filter to only symbols that need fetching
    std::vector<std::string> toFetch;
    size_t alreadyCached = 0;
    
    for (const auto &symbol : symbols) {
        if (symbol == "EUR") {
            alreadyCached++;
            continue;
        }
        
        if (cache_.isValid(symbol)) {
            alreadyCached++;
            continue;
        }
        
        toFetch.push_back(symbol);
    }
    
    std::cout << "\n[PriceCache] Cached: " << cache_.size() << "/" << symbols.size() 
              << " | Need to fetch: " << toFetch.size() 
              << " | Already cached: " << alreadyCached << "\n";
    
    if (toFetch.empty()) {
        std::cout << "[PriceCache] All prices up-to-date, no network fetch needed!\n";
        return true;
    }
    
    std::cout << "[PriceFetcher] Fetching " << toFetch.size() << " prices from CoinGecko...\n";
    
    size_t success = 0, failed = 0;
    auto startTime = std::chrono::steady_clock::now();
    
    // Fetch in single bulk request
    auto prices = priceProvider_->getPricesBulk(toFetch);
    
    for (const auto &[sym, price] : prices) {
        if (price > 0) {
            cache_.set(sym, price);
            success++;
        } else {
            failed++;
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(endTime - startTime).count();
    
    std::cout << "[PriceFetcher] Done: " << success << " succeeded, " 
              << failed << " failed, " << alreadyCached << " unchanged from cache\n"
              << "[PriceFetcher] Time: " << (int)elapsed << "s\n";
    
    return true; // Always return true - partial success is still success
}

bool AssetValue::refreshCache() {
    auto cached = cache_.getSymbols();
    if (cached.empty()) return true;
    
    std::cout << "[PriceCache] Refreshing " << cached.size() << " cached prices...\n";
    return loadPrices(cached);
}

bool AssetValue::isConnected() const {
    return isRunning_.load();
}

size_t AssetValue::cacheSize() const {
    return cache_.size();
}

void AssetValue::loadCache() {
    cache_.loadFromFile();
}

void AssetValue::saveCache() const {
    cache_.saveToFile();
}
