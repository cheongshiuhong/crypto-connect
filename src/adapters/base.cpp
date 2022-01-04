#include "adapters/base.hpp"
#include "strategies/base.hpp"

namespace Adapters::Base
{
    Adapter::Adapter(Strategies::Base::Strategy *const strategy)
    {
        this->strategy_ = strategy;
        this->strategy_->registerAdapter(this);
        this->strategy_->onInit();
    }
}
