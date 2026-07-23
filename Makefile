CC      ?= cc
CFLAGS  ?= -Wall -Wextra -std=c11
CFLAGS  += -Iinclude
LDLIBS  += -lm

LIB_SRC := \
    src/raven/raven_decode.c \
    src/rook/table_track.c \
    src/capture/capture_ble.c \
    src/capture/capture_file.c \
    src/pipeline/pipeline.c \
    src/magpie/magpie_validator.c \
    src/output/output_json.c \
    src/output/server.c
BUILD   := build

.PHONY: all test replay demo daemon clean
all: test replay demo daemon

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/test_decode: tests/unit/test_decode_full.c daemon/config.c $(LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/korax_replay: tools/replay/korax_replay.c $(LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/korax_demo: examples/demo_decode.c $(LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/koraxd: daemon/koraxd.c daemon/config.c daemon/log.c $(LIB_SRC) | $(BUILD)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@

test: $(BUILD)/test_decode
	./$(BUILD)/test_decode

replay: $(BUILD)/korax_replay
	./$(BUILD)/korax_replay tools/replay/frames.txt

demo: $(BUILD)/korax_demo
	./$(BUILD)/korax_demo

daemon: $(BUILD)/koraxd

clean:
	rm -rf $(BUILD)


