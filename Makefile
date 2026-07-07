CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SRC = src/decode/decode.c
DEMO = examples/demo_decode.c
TEST = tests/test_decode_full.c

APP = wingprint_demo
TEST_APP = test_decode

.PHONY: all test clean

all: $(APP)

$(APP): $(DEMO) $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_APP): $(TEST) $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_APP)
	./$(TEST_APP)

clean:
	rm -f $(APP) $(TEST_APP)
