CC = g++
CFLAGS = -std=c++2a
LIBS = -lboost_system -lssl -lcrypto
INCLUDE = -I /usr/include -I ../../include
PYTHON = -lpython3.9 -I /usr/include/python3.9
THREAD = -pthread
CRYPTOCONNECT = -Wl,-rpath,../../build -L. ../../build/libcryptoconnect-cbpro.so

main:
	$(CC) \
		-o main.o \
		downloader.cpp \
		filter.cpp \
		strategy.cpp \
		main.cpp \
		$(CFLAGS) \
		$(INCLUDE) \
		$(LIBS) \
		$(PYTHON) \
		$(THREAD) \
		${CRYPTOCONNECT}
