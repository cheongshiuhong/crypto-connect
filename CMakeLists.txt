cmake_minimum_required(VERSION 3.20)
project(cryptoconnect VERSION 1.0.1 DESCRIPTION "Crypto Connect")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-std=c++2a -fPIC")

# Headers
set(BASE_HEADERS
    include/cryptoconnect/strategy.hpp
    include/cryptoconnect/helpers/network/http/session.hpp
    include/cryptoconnect/helpers/network/websockets/client.hpp
    include/cryptoconnect/helpers/utils/base64.hpp
    include/cryptoconnect/helpers/utils/cryptography.hpp
    include/cryptoconnect/helpers/utils/datetime.hpp
    include/cryptoconnect/helpers/utils/exceptions.hpp
    include/cryptoconnect/structs/event_queue.hpp
    include/cryptoconnect/structs/events.hpp
    include/cryptoconnect/structs/orders.hpp
    include/cryptoconnect/structs/products.hpp
    include/cryptoconnect/structs/universe.hpp
    include/cryptoconnect/adapters/base.hpp
)

# Sources
set(BASE_SOURCES
    src/helpers/network/http/session.cpp
    src/helpers/network/websockets/client.cpp
    src/adapters/base.cpp
)

# Dependencies
set(BASE_DEPENDENCIES
    dependencies/yaml/yaml.cpp
)

# Exchange Argument
string( TOLOWER "${EXCHANGE}" exchange_lower )

if( "${exchange_lower} " STREQUAL "cbpro " OR "${exchange_lower} " STREQUAL "coinbasepro " ) 
    message("Building for CoinbasePro...")
    set(SO_SUFFIX "cbpro")
    set(EXCHANGE_HEADERS
        include/cryptoconnect/adapters/coinbasepro.hpp
        include/cryptoconnect/adapters/coinbasepro/adapter.hpp
        include/cryptoconnect/adapters/coinbasepro/auth.hpp
        include/cryptoconnect/adapters/coinbasepro/rest/connector.hpp
        include/cryptoconnect/adapters/coinbasepro/rest/bars_scheduler.hpp
        include/cryptoconnect/adapters/coinbasepro/stream/connector.hpp
        include/cryptoconnect/adapters/coinbasepro/stream/handler.hpp
    )
    set(EXCHANGE_SOURCES
        src/adapters/coinbasepro/rest/connector.cpp
        src/adapters/coinbasepro/rest/bars_scheduler.cpp
        src/adapters/coinbasepro/stream/handler.cpp
        src/adapters/coinbasepro/stream/connector.cpp
        src/adapters/coinbasepro/auth.cpp
        src/adapters/coinbasepro/adapter.cpp
    )

    elseif( "${exchange_lower} " STREQUAL "kucoin")
    message("Building for KuCoin...")
    set(SO_SUFFIX "kucoin")

else()
    message(SEND_ERROR)
    message("\tPlease specify which exchange the build is for. e.g., -DEXCHANGE=cbpro")
    message("\tCurrent available options are cbpro, kucoin.")
    message("")

endif()

# Sandbox Argument
string ( TOLOWER "${MODE} " mode_lower )
if( mode_lower STREQUAL "sandbox " )
    message("MODE: Sandbox...")
    set(IS_SANDBOX 1)
    message(${IS_SANDBOX})
    elseif( mode_lower STREQUAL "live ")
    message("MODE: Live...")
    set(IS_SANDBOX 0)
    message(${IS_SANDBOX})
else()
    message(SEND_ERROR)
    message("\tPlease specify if you are building for sandbox or not. e.g., -DMODE=sandbox")
    message("")
endif()


# Dependencies
set(OPENSSL_USE_STATIC_LIBS TRUE)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)

# Find the dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)

# Build
set(TARGET_NAME "cryptoconnect-${SO_SUFFIX}")

add_library(${TARGET_NAME} SHARED
    ${BASE_SOURCES} ${EXCHANGE_SOURCES}
    ${BASE_HEADERS} ${EXCHANGE_HEADERS}
    ${BASE_DEPENDENCIES} ${EXCHANGE_DEPENDENCIES}
)
target_link_libraries(${TARGET_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto ${CMAKE_THREAD_LIBS_INIT} ${BOOST_INCLUDE_LIBS})
target_include_directories(${TARGET_NAME} PRIVATE SYSTEM ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
target_include_directories(${TARGET_NAME} PUBLIC include dependencies)

# Macros
target_compile_definitions(${TARGET_NAME} PUBLIC IS_SANDBOX=${IS_SANDBOX})


SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "cryptoconnect")
SET(CPACK_PACKAGE_VENDOR "cheongshiuhong")
SET(CPACK_PACKAGE_VERSION "1.0.0")
SET(CPACK_GENERATOR "RPM;DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "N/A")
INCLUDE(CPack)
