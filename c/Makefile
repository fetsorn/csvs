CC       = cc
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -g -O2 -D_POSIX_C_SOURCE=200809L
AR       = ar
ARFLAGS  = rcs

# Library sources (everything except main.c and tests/)
LIB_SRCS = csvs_error.c csvs_grain.c csvs_entry.c csvs_schema.c \
           csvs_line.c csvs_version.c csvs_init.c csvs_dataset.c \
           csvs_query.c csvs_build.c csvs_option.c csvs_select.c \
           csvs_update.c csvs_insert.c csvs_delete.c csvs_prose.c
LIB_OBJS = $(LIB_SRCS:.c=.o) lib/libcsv/libcsv.o

# Build only the objects that exist so far
EXISTING_SRCS = $(wildcard csvs_*.c)
EXISTING_OBJS = $(EXISTING_SRCS:.c=.o)

.PHONY: all clean test lib

all: libcsvs.a

lib/libcsv/libcsv.o: lib/libcsv/libcsv.c lib/libcsv/csv.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c csvs.h csvs_internal.h
	$(CC) $(CFLAGS) -c -o $@ $<

libcsvs.a: $(EXISTING_OBJS) lib/libcsv/libcsv.o
	$(AR) $(ARFLAGS) $@ $^

# Test harness
TEST_SRCS  = tests/test_main.c
TEST_OBJS  = tests/test_main.o lib/cjson/cJSON.o
TEST_BIN   = tests/test_csvs

lib/cjson/cJSON.o: lib/cjson/cJSON.c lib/cjson/cJSON.h
	$(CC) $(CFLAGS) -w -c -o $@ $<

tests/test_main.o: tests/test_main.c csvs.h
	$(CC) $(CFLAGS) -I. -Ilib/cjson -c -o $@ $<

$(TEST_BIN): tests/test_main.o lib/cjson/cJSON.o libcsvs.a
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f *.o lib/libcsv/*.o lib/cjson/*.o tests/*.o
	rm -f libcsvs.a $(TEST_BIN)
