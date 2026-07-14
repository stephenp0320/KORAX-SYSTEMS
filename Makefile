CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SRC = \
      src/raven/raven_decode.c \
      src/capture/capture_ble.c \
      src/rook/table_track.c \
      src/pipeline/pipeline.c \

DEMO = examples/demo_decode.c
TEST = tests/test_decode_full.c
REPLAY = examples/korax_replay.c

APP = korax_demo
TEST_APP = test_decode

REPLAY_APP = korax_replay

.PHONY: all test replay clean  
all: $(APP)

$(APP): $(DEMO) $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(TEST_APP): $(TEST) $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(REPLAY_APP): $(REPLAY) $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

test: $(TEST_APP)
	./$(TEST_APP)

replay: $(REPLAY_APP)
	./$(REPLAY_APP) frames.txt

clean:
	rm -f $(APP) $(TEST_APP) $(REPLAY_APP)
