CC       = cc
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -g -O2 -D_POSIX_C_SOURCE=200809L
AR       = ar
ARFLAGS  = rcs

CJSON_CFLAGS = $(shell pkg-config --cflags libcjson)
CJSON_LIBS   = $(shell pkg-config --libs libcjson)

# Library sources (everything except main.c and tests/)
LIB_SRCS = csvs_error.c csvs_grain.c csvs_entry.c csvs_schema.c \
           csvs_line.c csvs_version.c csvs_init.c csvs_dataset.c \
           csvs_query.c csvs_build.c csvs_option.c csvs_select.c \
           csvs_update.c csvs_insert.c csvs_delete.c csvs_prose.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

# Build only the objects that exist so far
EXISTING_SRCS = $(wildcard csvs_*.c)
EXISTING_OBJS = $(EXISTING_SRCS:.c=.o)

.PHONY: all clean test lib

all: libcsvs.a

%.o: %.c csvs.h csvs_internal.h
	$(CC) $(CFLAGS) -c -o $@ $<

libcsvs.a: $(EXISTING_OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Test harness
TEST_SRCS  = tests/test_main.c
TEST_BIN   = tests/test_csvs

tests/test_main.o: tests/test_main.c csvs.h
	$(CC) $(CFLAGS) $(CJSON_CFLAGS) -I. -c -o $@ $<

$(TEST_BIN): tests/test_main.o libcsvs.a
	$(CC) $(CFLAGS) -o $@ $^ $(CJSON_LIBS) -lcsv -lm

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f *.o tests/*.o
	rm -f libcsvs.a $(TEST_BIN)
