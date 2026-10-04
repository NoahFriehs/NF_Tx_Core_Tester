#ifndef NF_TX_CORE_TESTER_PRICE_PROVIDER_H
#define NF_TX_CORE_TESTER_PRICE_PROVIDER_H

#include <string>

/**
 * Abstract base class for crypto price providers.
 * Mirrors BaseCryptoPrices.kt from Android.
 */
class PriceProvider {
public:
    virtual ~PriceProvider() = default;

    /**
     * Returns the price of the entered symbol (in EUR).
     * @param symbol The crypto symbol to get the price for.
     * @return The price in EUR, or 0.0 if no price was found.
     */
    virtual double getPrice(const std::string &symbol) = 0;
};

#endif //NF_TX_CORE_TESTER_PRICE_PROVIDER_H
