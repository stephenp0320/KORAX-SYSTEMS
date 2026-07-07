CC = gcc
 CFLAGS = -Wall -Wextra -Wpedantic -std=c11 -Iinclude

 SRC_DIR = src
 TEST_DIR = tests

 SRC = $(SRC_DIR)/remoteid_decoder.c
 MAIN = $(SRC_DIR)/main.c
 TEST = $(TEST_DIR)/test_decoder_full.c

 APP = remoteid_decoder
 TEST_APP = test_decoder

 .PHONY: all test clean rebuild

 all: $(APP)

 $(APP): $(MAIN) $(SRC)
	 $(CC) $(CFLAGS) $^ -o $@

 $(TEST_APP): $(TEST) $(SRC)
	 $(CC) $(CFLAGS) $^ -o $@

 test: $(TEST_APP)
	 ./$(TEST_APP)

 clean:
	 rm -f $(APP) $(TEST_APP)

 rebuild: clean all $(APP) $(TEST_APP)
