CFLAGS=-Wall -Werror -O2 -D_GNU_SOURCE

usb-led-matrix-hidraw: main.o
	$(CC) -o $@ $<

clean:
	rm -f usb-led-matrix-hidraw *.o
