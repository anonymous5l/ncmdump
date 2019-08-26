all:
	g++ main.cpp cJSON.cpp aes.cpp ncmcrypt.cpp -o ncmdump -ltag
	strip ncmdump

install: all
	cp fckncm /usr/local/bin
	mv ncmdump /usr/local/bin
