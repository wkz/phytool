.PHONY: all clean install

APPLETS = mv6tool

PREFIX ?= /usr/local/
LIBS    = 
CC      = gcc
CFLAGS  = -g -Wall -Wextra

objs = $(patsubst %.c, %.o, $(wildcard *.c))
hdrs = $(wildcard *.h)

%.o: %.c $(hdrs) Makefile
	@$(CROSS_COMPILE)$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@

phytool: $(objs)
	@$(CROSS_COMPILE)$(CC) $(objs) -Wall $(LIBS) -o $@

all: phytool

clean:
	@rm -f *.o
	@rm -f $(TARGET)

install: phytool
	@cp phytool $(DESTDIR)/$(PREFIX)/bin/
	@for app in $(APPLETS); do \
		ln -sf phytool $(DESTDIR)/$(PREFIX)/bin/$$app; \
	done
