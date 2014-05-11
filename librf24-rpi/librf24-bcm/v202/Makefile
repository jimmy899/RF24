CPPFLAGS = -I../
CFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall
LDFLAGS = -L.. -lrf24-bcm

all: v202_rx v202_tx

v202_rx: Raspduno.o v202_protocol.o v202_rx.c
	g++ ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} $^ -o $@

v202_tx: Raspduno.o v202_protocol.o v202_tx.c
	g++ ${CFLAGS} ${CPPFLAGS} ${LDFLAGS} $^ -o $@ -pthread

v202_protocol.o: v202_protocol.cpp
	g++ ${CFLAGS} ${CPPFLAGS} -fPIC -c $< -o $@

Raspduno.o: Raspduno.cpp
	g++ ${CFLAGS} ${CPPFLAGS} -fPIC -c $< -o $@

clean:
	rm -rf v202_rx v202_tx

run_rx:
	sudo LD_LIBRARY_PATH=.. ./v202_rx

run_tx:
	sudo LD_LIBRARY_PATH=.. ./v202_tx

.PHONY: install
