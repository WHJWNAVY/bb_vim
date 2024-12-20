TOP_DIR=$(shell pwd)
SRCDIR:=$(TOP_DIR)
OBJDIR:=$(TOP_DIR)
TARGET:=vi
CC?=gcc

CFLAGS+=-Os -Wall -Wextra
CFLAGS+=-I$(TOP_DIR)/include

SOURCES:=$(wildcard $(SRCDIR)/*.c $(SRCDIR)/libbb/*.c)
OBJECTS:=$(patsubst %.c, %.o, $(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f *.o libbb/*.o $(TARGET)

lint:
	find $(PRODUCT_DIR) -iname "*.[ch]" | xargs clang-format -i