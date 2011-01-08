OETMP = /oe7

BOARD = overo

SYSROOTSDIR=$(OETMP)/sysroots
STAGEDIR=$(SYSROOTSDIR)/x86_64-linux/usr
KERNELDIR=$(SYSROOTSDIR)/$(BOARD)-angstrom-linux-gnueabi/kernel

TOOLDIR = $(OETMP)/sysroots/x86_64-linux/usr/armv7a/bin
STAGEDIR = $(OETMP)/sysroots/armv7a-angstrom-linux-gnueabi/usr

CC = $(TOOLDIR)/arm-angstrom-linux-gnueabi-gcc
CFLAGS = -Wall

LIBDIR = $(STAGEDIR)/lib
INCDIR = $(STAGEDIR)/include

INC_PATH = -I $(KERNELDIR)/include -I $(INCDIR)

OBJS = main.o \
	feature_test.o 


TARGET = capture

$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) $(INC_PATH) -lpthread $(OBJS) -o $(TARGET)


main.o: main.c
	$(CC) $(CFLAGS) $(INC_PATH) -c main.c

feature_test.o: feature_test.c
	$(CC) $(CFLAGS) $(INC_PATH) -c feature_test.c


install:
	sudo cp $(TARGET) /exports/overo/home/root

clean:
	rm -rf $(TARGET) *.o

