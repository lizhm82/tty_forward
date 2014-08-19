
TARGETS = ttyforward

CROSS_COMPILE	:=
#CROSS_COMPILE	:=arm-none-linux-gnueabi-

CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip

.SUFFIXES:	.c .o

.c.o:
	$(CC) $(GLOBAL_DEFS) $(CFLAGS) -c $<

.cpp.o:
	$(CPP) $(GLOBAL_DEFS) $(CFLAGS) -c -o $@ $<

SRC_OBJS = main.o

.PHONY : all clean

all : $(TARGETS)

$(TARGETS) : $(SRC_OBJS)
	$(CC) -static -o $@ $^
	$(STRIP) $(TARGETS)


clean :
	-rm -f $(SRC_OBJS)
	-rm -f $(TARGETS)
