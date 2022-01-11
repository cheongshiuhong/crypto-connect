CC = g++
CFLAGS = -std=c++2a
LIBS = -lboost_system -lssl -lcrypto
INCLUDE = -I /usr/include -I ../../dependencies -I ../../include
PYTHON = -lpython3.9 -I /usr/include/python3.9
PFLAGS = -pthread

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
		$(PFLAGS) \
		-L. ./cbpro-adapter.so
