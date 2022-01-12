#ifndef PRODUCTS_PRODUCT_H
#define PRODUCTS_PRODUCT_H

#include <memory>
#include <string>
#include <unordered_map>

namespace Products
{
    struct Product
    {
        std::string id_;
        std::string name_;
        std::string baseCurrency_;
        std::string quoteCurrency_;
        double baseMinSize_;
        double baseMaxSize_;
        double baseIncrement_;
        double quoteIncrement_;
        bool isTradingEnabled_;
        bool isMarginEnabled_;

        Product() : id_(""), name_(""), baseCurrency_(""), quoteCurrency_(""),
                    baseMinSize_(0), baseMaxSize_(0),
                    baseIncrement_(0), quoteIncrement_(0),
                    isTradingEnabled_(false), isMarginEnabled_(false){};

        Product(std::string id, std::string name,
                std::string baseCurrency, std::string quoteCurrency,
                double baseMinSize, double baseMaxSize,
                double baseIncrement, double quoteIncrement,
                bool isTradingEnabled, bool isMarginEnabled)
            : id_(id), name_(name), baseCurrency_(baseCurrency), quoteCurrency_(quoteCurrency),
              baseMinSize_(baseMinSize), baseMaxSize_(baseMaxSize),
              baseIncrement_(baseIncrement), quoteIncrement_(quoteIncrement),
              isTradingEnabled_(isTradingEnabled), isMarginEnabled_(isMarginEnabled){};
    };

    using productPtr_t = std::shared_ptr<Product>;
    using productMap_t = std::unordered_map<std::string, productPtr_t>;
}

#endif
