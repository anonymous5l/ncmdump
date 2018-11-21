#!/bin/bash
LDFLAGS=-L/usr/local/opt/openssl/lib
CPPFLAGS=-I/usr/local/opt/openssl/include

all:
	g++ $(CPPFLAGS) main.cpp cJSON.cpp -o ncmdump -lcrypto -ltag -lz $(LDFLAGS)
	strip ncmdump

debug: 
	g++ $(CPPFLAGS) main.cpp cJSON.cpp -g -o ncmdump -lcrypto -ltag -lz $(LDFLAGS)
	
install:
	mv ncmdump /usr/local/bin/ncmdump

