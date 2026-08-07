/*
 * File/replay -- capture source.
 *
 * Reads hex-encoded frames from a text file and feeds them through the same
 * wp_frame_cb_t callback contract as the live captures.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "korax/raven_decode.h"
#include "korax/capture.h"

/*
 * Parses whitespace-separated hex byte pairs from one
 * line, stopping at the first token that doesn't parse or at `max` bytes.
 */
static int hexline_to_bytes(const char *line, uint8_t *out, int max){
	int n = 0;
	const char *pline = line;
	while (n < max){
		unsigned int byte;
		int consumed = 0;
		if (sscanf(pline, " %2x%n", &byte, &consumed) != 1) break;
		out[n++] = (uint8_t)byte;
		pline += consumed;
	}
	return n;
}

/* Takes a file and reads its contents */
FILE *read_file(const char *filename){
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL) return NULL;
    return fp;
}

/*
 * Reads `fp` line by line and decodes each hex
 * line into a frame, and invokes `cb` once per frame with medium set to
 * WP_MEDIUM_REPLAY.
 *
 * `frame.payload` points at a stack buffer scoped to a
 * single iteration -- `cb` must consume it synchronously and not retain the
 * pointer past the call. Returns the number of frames delivered, or -1 on
 * a read error.
 */
int get_file_line(FILE *fp, wp_frame_cb_t cb, void *user_data){
    if (fp == NULL || cb == NULL) return -1;

    char line[1024];
    int frames = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        uint8_t buf[256];
        int len = hexline_to_bytes(line, buf, sizeof(buf));

        if (len <= 0) continue;

        wp_frame_t frame = {0};
        frame.payload = buf;
        frame.payload_len = (size_t)len;
        frame.medium = WP_MEDIUM_REPLAY;
        cb(&frame, user_data);
        frames++;
    }

    if (ferror(fp)) return -1;
    return frames;
}

/*
 * Top-level replay driver: opens `path` and streams its frames through
 * get_file_line().
 *
 * Returns the frame count, or -1 if the file can't be
 * opened or a read error occurs.
 */
int wp_replay_run(const char *path, wp_frame_cb_t cb, void *user_data){
    if (path == NULL || cb == NULL) return -1;

    FILE *fp = read_file(path);
    if (fp == NULL) return -1;

    int frames = get_file_line(fp, cb, user_data);
    fclose(fp);
    return frames;
}
