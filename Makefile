TOP_DIR=$(shell pwd)
CFLAGS+=-Os -Wall -Wextra -std=gnu99
CFLAGS+=-Wformat -Werror=format-security
# CFLAGS+=-Werror=format-nonliteral
CFLAGS+=-Wno-error=unused-result
CFLAGS+=-Wno-error=unused-but-set-variable
CFLAGS+=-Werror=implicit-function-declaration
CFLAGS+=-Wmissing-declarations -Wno-unused-parameter
CFLAGS+=-I$(TOP_DIR)/include

all: vi

vi: libbb/ptr_to_globals.o libbb/verror_msg.o libbb/xfuncs.o libbb/xfuncs_printf.o libbb/read_key.o libbb/read.o vi.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

clean:
	rm -f *.o libbb/*.o vi

lint:
	find $(PRODUCT_DIR) -iname "*.[ch]" | xargs clang-format -i