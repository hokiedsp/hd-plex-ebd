#* Microdia Eject Device User Space USB HID driver Makefile.
#* 
#* Version:     1.1
#*
#* Authors:     Takeshi Ikuma & Arsalan Masood
#*
#* Version History:=
#*    1.0 - original release (Masood)
#*    1.1 - library dependency changed from libusb-1.0 to libudev (Ikuma)
#*
#*/

SRCS=ejectd.c
OBJS=$(patsubst %c,%o, $(SRCS))

all: ejectd

ejectd: $(OBJS)
	$(CC) $(OBJS) -o $@ -ludev

%.o: %.c
	$(CC) -c $< -o $@

install: ejectd ejectd.sh
	cp ejectd /usr/sbin
	cp ejectd.sh /etc/init.d
	chmod +x /etc/init.d/ejectd.sh
	update-rc.d ejectd.sh defaults

uninstall:
	rm -f /usr/sbin/ejectd /etc/init.d/ejectd.sh
	find /etc/rc* -name "*ejectd*" | xargs rm -f

.PHONY: clean
clean:
	rm -rf $(OBJS) ejectd

