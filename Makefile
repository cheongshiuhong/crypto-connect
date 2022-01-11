CC = g++
CFLAGS = -std=c++2a -fPIC
LDFLAGS = -shared
LIBS = -lboost_system -lssl -lcrypto
INCLUDE = -I /usr/include -I dependencies -I include/cryptoconnect
PFLAGS = -pthread

cbpro:
	$(CC) \
		-o cbpro-adapter.so \
		dependencies/yaml/yaml.cpp \
		src/helpers/network/http/session.cpp \
		src/helpers/network/websockets/client.cpp \
		src/adapters/base.cpp \
		src/adapters/coinbasepro/rest/connector.cpp \
		src/adapters/coinbasepro/rest/bars_scheduler.cpp \
		src/adapters/coinbasepro/stream/handler.cpp \
		src/adapters/coinbasepro/stream/connector.cpp \
		src/adapters/coinbasepro/auth.cpp \
		src/adapters/coinbasepro/adapter.cpp \
		-g \
		$(CFLAGS) \
		$(LDFLAGS) \
		$(INCLUDE) \
		$(LIBS) \
		$(PFLAGS)
