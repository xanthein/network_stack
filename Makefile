TARGET	:= stack_test
SOURCES	:= utils.c arp.c ipv4.c main.c netdev.c tap_if.c icmpv4.c tcpv4.c
CFLAGS	:=
LFLAGS	:=
LIBS	:=

objects := $(SOURCES:.c=.o)

.PHONY: all, clean

all: $(TARGET)
clean:
	rm -f $(TARGET)
	rm *.o

$(TARGET): Makefile $(objects)
	$(CC) $(LFLAGS) -o $@ $(objects) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
