# Makefile to build ntp_raw (the program that queries an NTP server using raw
# sockets) and print_ip (the program that prints the IP address for a network
# interface.
#
# This Makefile is as portable and bug-free as all other hand-written ad-hoc
# makefiles, if not less so.
#
# Author: Denis Kobozev <d.v.kobozev@gmail.com>

CC = gcc
CFLAGS += -Wall
PRINT_IP_OBJS := ip.o print_ip.o
NTP_RAW_OBJS := ip.o checksum.o ntp_raw.o

.PHONY: all clean distclean

all: print_ip ntp_raw

print_ip: $(PRINT_IP_OBJS)

ntp_raw: $(NTP_RAW_OBJS)

clean:
	@- $(RM) print_ip ntp_raw
	@- $(RM) $(PRINT_IP_OBJS) $(NTP_RAW_OBJS)
