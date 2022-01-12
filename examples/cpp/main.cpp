#include "cryptoconnect/adapters/coinbasepro.hpp"
#include "./strategy.hpp"

int main()
{
    CBProStrategy strategy;
    CryptoConnect::CoinbasePro::Adapter adapter(&strategy);
    adapter.start();
}
