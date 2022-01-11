#include "adapters/coinbasepro/adapter.hpp"
#include "./strategy.hpp"

int main()
{
    CBProStrategy strategy;
    Adapters::CoinbasePro::Adapter adapter(&strategy);
    adapter.start();
}
