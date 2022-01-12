#ifndef UTILS_EXCEPTIONS_H
#define UTILS_EXCEPTIONS_H

#include <functional>

namespace Utils::Exceptions
{
    inline void withHandler(
        std::function<void()> func,
        std::function<void()> callback,
        std::string message)
    {
        try
        {
            func();
        }
        catch (const std::exception &e)
        {
            std::cerr << message << '\n';
            std::cerr << e.what() << '\n';
            callback();
            exit(1);
        }
    }
}

#endif
