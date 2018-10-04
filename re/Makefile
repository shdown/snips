PROGRAM := main
SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))
CFLAGS := -Wall -Wextra -std=c99 -O2
CPPFLAGS := -D_POSIX_C_SOURCE=200809L -DPROGRAM_NAME=\"$(PROGRAM)\"

$(PROGRAM): $(OBJECTS)

clean:
	$(RM) $(PROGRAM) $(OBJECTS)

.PHONY: clean
