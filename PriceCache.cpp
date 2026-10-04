#include "PriceCache.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <regex>

PriceCache::PriceCache() {}

PriceCache::~PriceCache() {
    saveToFile();
}

void PriceCache::loadFromFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(CACHE_FILE);
    if (!file.is_open()) {
        std::cout << "No cached prices file found, will fetch all from network\n";
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    
    if (content.empty()) {
        std::cout << "Cached prices file is empty\n";
        return;
    }
    
    cache_ = parseJson(content);
    
    // Filter out expired entries
    auto now = std::time(nullptr);
    for (auto it = cache_.begin(); it != cache_.end();) {
        if (now - it->second.timestamp > 86400) {
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
    
    std::cout << "Loaded " << cache_.size() << " prices from cache\n";
}

void PriceCache::saveToFile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream file(CACHE_FILE);
    if (!file.is_open()) {
        std::cerr << "Failed to save prices to cache file\n";
        return;
    }
    
    std::string json = serializeJson(cache_);
    file << json;
    file.close();
}

double PriceCache::get(const std::string &symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(symbol);
    if (it == cache_.end()) return -1.0;
    
    auto now = std::time(nullptr);
    if (now - it->second.timestamp > 86400) {
        return -1.0;  // Expired
    }
    
    return it->second.price;
}

void PriceCache::set(const std::string &symbol, double price) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::time(nullptr);
    cache_[symbol] = {price, static_cast<int64_t>(now)};
    
    // Save immediately so next run can use it
    std::ofstream file(CACHE_FILE);
    if (file.is_open()) {
        std::string json = serializeJson(cache_);
        file << json;
        file.close();
    }
}

bool PriceCache::isValid(const std::string &symbol) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(symbol);
    if (it == cache_.end()) return false;
    
    auto now = std::time(nullptr);
    return now - it->second.timestamp <= 86400;
}

std::vector<std::string> PriceCache::getSymbols() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> symbols;
    auto now = std::time(nullptr);
    
    for (const auto &[sym, entry] : cache_) {
        if (now - entry.timestamp <= 86400) {
            symbols.push_back(sym);
        }
    }
    
    return symbols;
}

size_t PriceCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::time(nullptr);
    size_t valid = 0;
    
    for (const auto &[sym, entry] : cache_) {
        if (now - entry.timestamp <= 86400) {
            valid++;
        }
    }
    
    return valid;
}

void PriceCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    
    std::ofstream file(CACHE_FILE);
    file << "{}";
    file.close();
}

// Simple JSON parser for our price cache format
std::unordered_map<std::string, PriceCache::CacheEntry> PriceCache::parseJson(const std::string &json) {
    std::unordered_map<std::string, CacheEntry> result;
    
    // Find all symbol entries: "SYMBOL": {"price": X, "time": Y}
    size_t pos = 0;
    while ((pos = json.find("\"", pos)) != std::string::npos) {
        size_t closingQuote = json.find("\"", pos + 1);
        if (closingQuote == std::string::npos) break;
        
        std::string symbol = json.substr(pos + 1, closingQuote - pos - 1);
        
        // Look for price and time values after this symbol
        size_t bracePos = json.find("{", closingQuote);
        if (bracePos == std::string::npos) break;
        
        size_t pricePos = json.find("\"price\"", bracePos);
        size_t timePos = json.find("\"time\"", bracePos);
        
        if (pricePos == std::string::npos && timePos == std::string::npos) {
            pos = closingQuote + 1;
            continue;
        }
        
        // Find price value
        double price = 0.0;
        if (pricePos != std::string::npos) {
            size_t valueStart = json.find_first_of("0123456789.-", pricePos + 7);
            if (valueStart != std::string::npos) {
                size_t valueEnd = json.find_first_not_of("0123456789.eE-+", valueStart + 1);
                if (valueEnd == std::string::npos) valueEnd = json.length();
                try {
                    price = std::stod(json.substr(valueStart, valueEnd - valueStart));
                } catch (...) {
                    price = 0.0;
                }
            }
        }
        
        // Find time value
        int64_t time = 0;
        if (timePos != std::string::npos) {
            size_t valueStart = json.find_first_of("0123456789", timePos + 6);
            if (valueStart != std::string::npos) {
                size_t valueEnd = json.find_first_not_of("0123456789", valueStart + 1);
                if (valueEnd == std::string::npos) valueEnd = json.length();
                try {
                    time = std::stoll(json.substr(valueStart, valueEnd - valueStart));
                } catch (...) {
                    time = 0;
                }
            }
        }
        
        if (price > 0 && time > 0) {
            result[symbol] = {price, time};
        }
        
        pos = closingQuote + 1;
    }
    
    return result;
}

// Simple JSON serializer
std::string PriceCache::serializeJson(const std::unordered_map<std::string, CacheEntry> &data) {
    std::string json = "{\n";
    bool first = true;
    
    for (const auto &pair : data) {
        const std::string &symbol = pair.first;
        const CacheEntry &entry = pair.second;
        
        if (!first) json += ",\n";
        first = false;
        
        json += "  \"" + symbol + "\": {";
        json += "\"price\": ";
        char priceBuf[64];
        snprintf(priceBuf, sizeof(priceBuf), "%f", entry.price);
        json += priceBuf;
        json += ", ";
        json += "\"time\": ";
        json += std::to_string(entry.timestamp);
        json += "}";
    }
    
    if (!data.empty()) json += "\n";
    json += "}";
    return json;
}
