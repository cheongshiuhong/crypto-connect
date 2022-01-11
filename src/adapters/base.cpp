#include "adapters/base.hpp"
#include "strategy.hpp"

namespace CryptoConnect
{
    BaseAdapter::BaseAdapter(BaseStrategy *const strategy)
    {
        this->strategy_ = strategy;
        this->strategy_->registerAdapter(this);
        this->strategy_->onInit();
    }
}
