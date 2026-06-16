CC       = cc
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -g -O2 -D_POSIX_C_SOURCE=200809L
AR       = ar
ARFLAGS  = rcs

CJSON_CFLAGS = $(shell pkg-config --cflags libcjson)
CJSON_LIBS   = $(shell pkg-config --libs libcjson)

CFLAGS  += $(CJSON_CFLAGS)

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

tests/test_main.o: tests/test_main.c csvs.h ../test/csvs_test.h
	$(CC) $(CFLAGS) -I. -I../test -c -o $@ $<

$(TEST_BIN): tests/test_main.o libcsvs.a
	$(CC) $(CFLAGS) -o $@ $^ $(CJSON_LIBS) -lcsv -lm

test: $(TEST_BIN)
	./$(TEST_BIN)

# Sanitizer build: make sanitize
.PHONY: sanitize
sanitize: CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
sanitize: clean $(TEST_BIN)
	./$(TEST_BIN)

# Valgrind: make valgrind
.PHONY: valgrind
valgrind: $(TEST_BIN)
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(TEST_BIN)

# CodeChecker: make check
.PHONY: check
check:
	rm -rf .codechecker
	mkdir -p .codechecker
	CodeChecker log -b "make -B" -o .codechecker/compile_commands.json
	CodeChecker analyze .codechecker/compile_commands.json -o .codechecker/reports
	CodeChecker parse .codechecker/reports

clean:
	rm -f *.o tests/*.o
	rm -f libcsvs.a $(TEST_BIN)
	rm -rf .codechecker
