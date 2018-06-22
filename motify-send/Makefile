PROGRAM := motify-send
PKGCONFIG_LIBS := glib-2.0 gio-2.0
SOURCES := $(wildcard *.c)
HEADERS := $(wildcard *.h)
CFLAGS := -Wall -Wextra -std=c99 -O2 $(shell pkg-config --cflags $(PKGCONFIG_LIBS))
CPPFLAGS := -D_POSIX_C_SOURCE=200809L -DPROGRAM_NAME=\"$(PROGRAM)\"
LDFLAGS := $(shell pkg-config --libs $(PKGCONFIG_LIBS))

$(PROGRAM): $(SOURCES) $(HEADERS)

clean:
	$(RM) $(PROGRAM)

.PHONY: clean
