#include "CoinGeckoPriceProvider.h"
#include "StaticPrices.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cmath>
#include <unordered_map>

// Simple JSON parser - extracts value for a given key from a JSON string

// Simple JSON parser - extracts value for a given key from a JSON string
static std::string extractJsonValue(const std::string &json, const std::string &key) {
    std::string searchKey = "\"" + key + "\"";
    auto startPos = json.find(searchKey);
    if (startPos == std::string::npos) return "";
    
    startPos = json.find(':', startPos + searchKey.length());
    if (startPos == std::string::npos) return "";
    startPos++;
    
    while (startPos < (int)json.length() && (json[startPos] == ' ' || json[startPos] == '\t')) {
        startPos++;
    }
    
    if (startPos < (int)json.length() && json[startPos] == '"') {
        startPos++;
        auto endPos = json.find('"', startPos);
        if (endPos == std::string::npos) return "";
        return json.substr(startPos, endPos - startPos);
    } else {
        auto endPos = json.find_first_of(",}", startPos);
        if (endPos == std::string::npos) endPos = json.length();
        return json.substr(startPos, endPos - startPos);
    }
}

// Extract HTTP status from curl output (last line after newline)
static std::string extractStatus(const std::string &output) {
    auto pos = output.rfind('\n');
    if (pos == std::string::npos) return "0";
    std::string status = output.substr(pos + 1);
    status.erase(0, status.find_first_not_of(" \r\n\t"));
    status.erase(status.find_last_not_of(" \r\n\t") + 1);
    return status;
}

bool CoinGeckoPriceProvider::isSafeSymbol(const std::string &symbol) {
    // Symbol values are interpolated into a shell command (curl). Only accept
    // ordinary asset symbols; anything else falls back to the static price.
    if (symbol.empty() || symbol.size() > 24) return false;
    for (char c: symbol) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-'))
            return false;
    }
    return true;
}

CoinGeckoPriceProvider::CoinGeckoPriceProvider() {
    loadSymbolCache();
}

void CoinGeckoPriceProvider::loadSymbolCache() {
    std::ifstream file(CACHE_FILE_IDS);
    if (!file.is_open()) return;
    
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    
    size_t pos = 0;
    while ((pos = content.find("\"", pos)) != std::string::npos) {
        size_t endQuote = content.find("\"", pos + 1);
        if (endQuote == std::string::npos) break;
        std::string symbol = content.substr(pos + 1, endQuote - pos - 1);
        
        size_t colonPos = content.find(":", endQuote);
        if (colonPos == std::string::npos) break;
        
        size_t valStart = content.find("\"", colonPos + 1);
        if (valStart == std::string::npos) break;
        size_t valEnd = content.find("\"", valStart + 1);
        if (valEnd == std::string::npos) break;
        
        std::string id = content.substr(valStart + 1, valEnd - valStart - 1);
        symbolIdCache_[symbol] = id;
        pos = valEnd + 1;
    }
}

void CoinGeckoPriceProvider::saveSymbolCache() {
    std::ofstream file(CACHE_FILE_IDS);
    if (!file.is_open()) return;
    
    file << "{";
    bool first = true;
    for (const auto &[sym, id] : symbolIdCache_) {
        if (!first) file << ",";
        file << "\"" << sym << "\":\"" << id << "\"";
        first = false;
    }
    file << "}";
    file.close();
}

std::string CoinGeckoPriceProvider::resolveSymbolToId(const std::string &symbol) {
    std::string mapped = symbolToCoinGeckoId(symbol);
    std::string lower = symbol;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (mapped != lower) return mapped;

    if (symbolIdCache_.count(symbol)) {
        return symbolIdCache_[symbol];
    }

    std::string url = "https://api.coingecko.com/api/v3/search?query=" + symbol;
    std::string cmd = "curl -s -m 10 '" + url + "' 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) return mapped;

    char buffer[8192];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    size_t coinsPos = result.find("\"coins\"");
    if (coinsPos != std::string::npos) {
        size_t startArray = result.find("[", coinsPos);
        size_t endArray = result.find("]", startArray);
        if (startArray != std::string::npos && endArray != std::string::npos) {
            std::string coinsJson = result.substr(startArray + 1, endArray - startArray - 1);
            
            // Split coins by "},{"
            size_t objPos = 0;
            while ((objPos = coinsJson.find("{", objPos)) != std::string::npos) {
                size_t objEnd = coinsJson.find("}", objPos);
                if (objEnd == std::string::npos) break;
                
                std::string coinObj = coinsJson.substr(objPos, objEnd - objPos + 1);
                
                // Extract symbol and id
                auto extract = [&](const std::string &key) {
                    size_t kPos = coinObj.find("\"" + key + "\"");
                    if (kPos == std::string::npos) return std::string("");
                    size_t vStart = coinObj.find("\"", coinObj.find(":", kPos) + 1);
                    if (vStart == std::string::npos) return std::string("");
                    size_t vEnd = coinObj.find("\"", vStart + 1);
                    if (vEnd == std::string::npos) return std::string("");
                    return coinObj.substr(vStart + 1, vEnd - vStart - 1);
                };
                
                std::string foundSymbol = extract("symbol");
                std::string foundId = extract("id");
                
                std::string lowerFoundSymbol = foundSymbol;
                std::transform(lowerFoundSymbol.begin(), lowerFoundSymbol.end(), lowerFoundSymbol.begin(), ::tolower);
                
                if (lowerFoundSymbol == lower) {
                    symbolIdCache_[symbol] = foundId;
                    saveSymbolCache();
                    return foundId;
                }
                
                objPos = objEnd + 1;
            }
        }
    }
    return mapped;
}

void CoinGeckoPriceProvider::checkRateLimit() {
    time_t now = std::time(nullptr);
    std::lock_guard<std::mutex> lock(rateLimitMutex_);
    
    if (rateLimited_ && now > rateLimitUntil_) {
        rateLimited_ = false;
    }
}

bool CoinGeckoPriceProvider::isRateLimited() const {
    std::lock_guard<std::mutex> lock(rateLimitMutex_);
    return rateLimited_;
}

void CoinGeckoPriceProvider::setRateLimited(time_t until) {
    std::lock_guard<std::mutex> lock(rateLimitMutex_);
    rateLimited_ = true;
    rateLimitUntil_ = until;
}

std::string CoinGeckoPriceProvider::symbolToCoinGeckoId(const std::string &symbol) {
    // Map common symbols to CoinGecko canonical IDs
    static const std::map<std::string, std::string> symbolMap = {
        {"BTC", "bitcoin"}, {"ETH", "ethereum"}, {"DOGE", "dogecoin"},
        {"CRO", "crypto-com-chain"}, {"EUR", "eur"}, {"ETHW", "ethereum-pow-iou"},
        {"LUNA2", "terra-luna-2"}, {"LUNC", "terra-luna"}, {"ALGO", "algorand"},
        {"XRP", "ripple"}, {"SOL", "solana"}, {"BNB", "binancecoin"},
        {"DOT", "polkadot"}, {"ADA", "cardano"}, {"LTC", "litecoin"},
        {"TRX", "tron"}, {"SHIB", "shiba-inu"}, {"XMR", "monero"},
        {"USDT", "tether"}, {"BCH", "bitcoin-cash"}, {"MATIC", "polygon"},
        {"LINK", "chainlink"}, {"UNI", "uniswap"}, {"ATOM", "cosmos"},
        {"AVAX", "avalanche-2"}, {"FIL", "filecoin"}, {"APT", "aptos"},
        {"ARB", "arbitrum"}, {"OP", "optimism"}, {"SUI", "sui"},
        {"PEPE", "pepe"}, {"WIF", "dogwifcoin"}, {"BONK", "bonk"},
        {"CAKE", "pancakeswap-token"}, {"VET", "vechain"}, {"ICP", "internet-computer"},
        {"ETC", "ethereum-classic"}, {"XLM", "stellar"}, {"NEAR", "near"},
        {"FTM", "fantom"}, {"HBAR", "hedera-hashgraph"}, {"FLOW", "flow"},
        {"IMX", "immutable-x"}, {"MANA", "decentraland"}, {"SAND", "the-sandbox"},
        {"AAVE", "aave"}, {"GRT", "the-graph"}, {"ENJ", "enjincoin"},
        {"CHZ", "chiliz"}, {"THETA", "theta-network"}, {"AXS", "axie-infinity"},
        {"XTZ", "tezos"}, {"EGLD", "elrond-erd-2"}, {"ZEC", "zcash"},
        {"DASH", "dash"}, {"RUNE", "thorchain"}, {"KSM", "kusama"},
        {"CELO", "celo"}, {"MINA", "mina-protocol"}, {"ZIL", "zilliqa"},
        {"WAVES", "waves"}, {"ROSE", "oasis-network"}, {"KLAY", "klay-token"},
        {"ONE", "harmony"}, {"BAT", "basic-attention-token"},
    };
    
    auto it = symbolMap.find(symbol);
    if (it != symbolMap.end()) {
        return it->second;
    }
    
    // Default: lowercase the symbol (works for many tokens)
    std::string result = symbol;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

double CoinGeckoPriceProvider::getPrice(const std::string &symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isSafeSymbol(symbol)) return ::getPrice(symbol);

    if (consecutiveRateLimits_ >= 4) {
        std::cout << "  [⚠ Too many rate limits. Falling back to static price for " << symbol << "]\n";
        return ::getPrice(symbol);
    }

    // Rate limiting
    time_t now = std::time(nullptr);
    recentCalls_.erase(
        std::remove_if(recentCalls_.begin(), recentCalls_.end(),
                       [now](const CallInfo &c) { return difftime(now, c.timestamp) > 60; }),
        recentCalls_.end());
    
    if (recentCalls_.size() >= MAX_CALLS_PER_MINUTE) {
        std::cout << "  [⏳ Rate limited, waiting " << COOLDOWN_SECONDS << "s...]\n";
        std::this_thread::sleep_for(std::chrono::seconds(COOLDOWN_SECONDS));
        return getPrice(symbol);
    }
    
    recentCalls_.push_back({symbol, now});
    
    std::string coinId = resolveSymbolToId(symbol);
    std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=" + coinId + "&vs_currencies=eur";
    
    std::string cmd = "curl -s -m 10 -w '\\n%{http_code}' '" + url + "' 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "  [✗ Failed to execute curl for " << symbol << "]\n";
        return 0.0;
    }
    
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    
    std::string httpStatus = extractStatus(result);
    std::string jsonStr = result.substr(0, result.rfind('\n'));
    
    if (httpStatus == "429") {
        consecutiveRateLimits_++;
        std::cerr << "  [⚠ Rate limited for " << symbol << "]\n";
        if (consecutiveRateLimits_ >= 4) {
            std::cout << "  [⚠ Too many rate limits. Falling back to static price for " << symbol << "]\n";
            return ::getPrice(symbol);
        }
        return 0.0;
    }
    
    consecutiveRateLimits_ = 0; // Reset on success
    if (httpStatus == "404") {
        return 0.0;
    }
    if (jsonStr.find("\"error\"") != std::string::npos) {
        std::cerr << "  [✗ API error for " << symbol << "]\n";
        return 0.0;
    }
    
    double price = 0.0;
    // Parse {"coinid":{"eur":price}}
    size_t eurPos = jsonStr.find("\"eur\"");
    if (eurPos != std::string::npos) {
        size_t valueStart = jsonStr.find_first_of("0123456789.-", eurPos + 5);
        if (valueStart != std::string::npos) {
            size_t valueEnd = jsonStr.find_first_not_of("0123456789.e-+", valueStart + 1);
            if (valueEnd == std::string::npos) valueEnd = jsonStr.length();
            try {
                price = std::stod(jsonStr.substr(valueStart, valueEnd - valueStart));
            } catch (...) {
                price = 0.0;
            }
        }
    }
    
    return price;
}

std::map<std::string, double> CoinGeckoPriceProvider::getPricesBulk(const std::vector<std::string> &symbols) {
    if (symbols.empty()) return {};

    std::map<std::string, double> allPrices;
    const size_t BATCH_SIZE = 30;  // Smaller batches to avoid rate limits
    int consecutiveFailures = 0;
    
    for (size_t batchStart = 0; batchStart < symbols.size(); batchStart += BATCH_SIZE) {
        size_t batchEnd = std::min(batchStart + BATCH_SIZE, symbols.size());
        size_t batchSize = batchEnd - batchStart;
        
        if (consecutiveRateLimits_ >= 4) {
            std::cout << "  [⚠ Too many rate limits. Falling back to static prices for this batch]\n";
            for (size_t j = batchStart; j < batchEnd; j++) {
                allPrices[symbols[j]] = ::getPrice(symbols[j]);
            }
            continue;
        }

        // Build comma-separated coin IDs and a mapping back to symbols
        std::string idList;
        std::map<std::string, std::string> idToSymbol;
        for (size_t j = batchStart; j < batchEnd; j++) {
            std::string symbol = symbols[j];
            if (!isSafeSymbol(symbol)) {
                allPrices[symbol] = ::getPrice(symbol);
                continue;
            }
            std::string coinId = resolveSymbolToId(symbol);
            if (!idList.empty()) idList += ",";
            idList += coinId;
            idToSymbol[coinId] = symbol;
            
            if (j % 5 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        }
        
        // Use single endpoint with comma-separated IDs for bulk
        std::string url = "https://api.coingecko.com/api/v3/simple/price?ids=" + idList + "&vs_currencies=eur";
        
        // DEBUG: Log what we are requesting
        std::cout << "  [DEBUG] Requesting: " << idList << "\n";
        
        std::string cmd = "curl -s -m 15 -w '\\n%{http_code}' '" + url + "' 2>/dev/null";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe) continue;
        
        char buffer[8192];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
        
        pclose(pipe);
        
        std::string httpStatus = extractStatus(output);
        std::string jsonStr = output.substr(0, output.rfind('\n'));
        
        // DEBUG: Log the response
        std::cout << "  [DEBUG] Response (" << httpStatus << "): " << jsonStr << "\n";
        
        if (httpStatus == "429") {
            consecutiveRateLimits_++;
            consecutiveFailures++;
            std::cout << "  [⚠ Rate limited (global attempt " << consecutiveRateLimits_ << "), backing off...]\n";
            
            // Exponential backoff with max 10 minutes
            int waitTime = std::min((int)std::pow(2, consecutiveFailures), 600);
            std::cout << "  [⏳ Waiting " << waitTime << "s before retry...]\n";
            std::this_thread::sleep_for(std::chrono::seconds(waitTime));
            
            // If too many consecutive failures, break out to avoid infinite retry
            if (consecutiveFailures > 3) {
                std::cout << "  [✗ Too many rate limit hits, stopping price fetch]\n";
                break;
            }
            continue;
        }
        
        consecutiveRateLimits_ = 0; // Reset on success
        consecutiveFailures = 0;  // Reset on success
        
        if (httpStatus == "404") {
            consecutiveFailures = 0;
            continue;
        }
        if (jsonStr.find("\"error\"") != std::string::npos) {
            std::cerr << "  [✗ API error during bulk fetch]\n";
            continue;
        }
        
        // Parse JSON: {"bitcoin":{"eur":price},"ethereum":{"eur":price},...}
        size_t pos = 0;
        int parsed = 0;
        while ((pos = jsonStr.find("\"", pos)) != std::string::npos) {
            size_t endQuote = jsonStr.find("\"", pos + 1);
            if (endQuote == std::string::npos) break;
            
            std::string coinId = jsonStr.substr(pos + 1, endQuote - pos - 1);
            
            // Find "eur" key within this object
            size_t afterObjStart = jsonStr.find("{", endQuote);
            if (afterObjStart == std::string::npos) { pos = endQuote + 1; continue; }
            
            size_t eurKeyPos = jsonStr.find("\"eur\"", afterObjStart);
            if (eurKeyPos == std::string::npos) { pos = endQuote + 1; continue; }
            
            // Extract price value
            size_t valueStart = jsonStr.find_first_of("0123456789.-", eurKeyPos + 5);
            if (valueStart == std::string::npos) { pos = endQuote + 1; continue; }
            
            size_t valueEnd = jsonStr.find_first_not_of("0123456789.eE-+", valueStart + 1);
            if (valueEnd == std::string::npos) valueEnd = jsonStr.length();
            
            double price = 0.0;
            try {
                price = std::stod(jsonStr.substr(valueStart, valueEnd - valueStart));
            } catch (...) {
                price = 0.0;
            }
            
            // Map coinId back to original symbol
            auto it = idToSymbol.find(coinId);
            if (it != idToSymbol.end()) {
                allPrices[it->second] = price;
                parsed++;
            }
            
            // Skip to end of this object to avoid nested parsing
            pos = jsonStr.find("}", endQuote);
            if (pos == std::string::npos) break;
            pos++;
        }
        
        std::cout << "  [✓ Fetched " << parsed << "/" << batchSize 
                  << " in batch (batch " << (batchStart / BATCH_SIZE + 1) 
                  << "/" << ((symbols.size() + BATCH_SIZE - 1) / BATCH_SIZE) << ")]\n";
        
        // Small delay between batches
        if (batchEnd < symbols.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(600));
        }
    }
    
    return allPrices;
}
