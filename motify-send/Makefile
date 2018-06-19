PROGRAM := motify-send
SOURCES := $(wildcard *.c)
HEADERS := $(wildcard *.h)
CFLAGS := -Wall -Wextra -std=c99 -O2
CPPFLAGS := -D_POSIX_C_SOURCE=200809L -DPROGRAM_NAME=\"$(PROGRAM)\"
LDFLAGS := $(shell pkg-config --libs --cflags glib-2.0 gio-2.0)

$(PROGRAM): $(SOURCES) $(HEADERS)

clean:
	$(RM) $(PROGRAM)

.PHONY: clean
