CFLAGS = -std=c++2a
LIBS = -lboost_system -lssl -lcrypto
INCLUDE = -I /usr/include -I dependencies -I include
PFLAGS = -pthread

coinbasepro:
	g++ \
		dependencies/yaml/yaml.cpp \
		dependencies/simdjson/simdjson.cpp \
		src/helpers/network/http/session.cpp \
		src/helpers/network/websockets/client.cpp \
		src/adapters/base.cpp \
		src/adapters/coinbasepro/rest/connector.cpp \
		src/adapters/coinbasepro/rest/bars_scheduler.cpp \
		src/adapters/coinbasepro/stream/handler.cpp \
		src/adapters/coinbasepro/stream/connector.cpp \
		src/adapters/coinbasepro/auth.cpp \
		src/adapters/coinbasepro/adapter.cpp \
		cbmain.cpp \
		$(CFLAGS) \
		$(INCLUDE) \
		$(LIBS) \
		$(PFLAGS) \
		-o cbmain.o
