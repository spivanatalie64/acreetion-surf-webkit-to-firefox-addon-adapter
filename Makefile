CC ?= gcc
PKG := gtk+-3.0 webkit2gtk-4.1 json-glib-1.0
CFLAGS := -fPIC -Wall -Wextra -O2 $(shell pkg-config --cflags $(PKG))
LDFLAGS := $(shell pkg-config --libs $(PKG))
SRCDIR := src
INCDIR := include
BUILDDIR := build
TARGET := $(BUILDDIR)/libacrefxadapter.so

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(SRCS:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

all: $(TARGET)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/lib $(DESTDIR)$(PREFIX)/include/acrefxadapter
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/lib/
	install -m 644 $(INCDIR)/*.h $(DESTDIR)$(PREFIX)/include/acrefxadapter/

clean:
	rm -rf $(BUILDDIR)

.PHONY: all install clean
