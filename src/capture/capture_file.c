#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "korax/raven_decode.h"
#include "korax/capture.h"

static int hexline_to_bytes(const char *line, uint8_t *out, int max){
	int n = 0;
	const char *p = line;
	while (n < max){
		unsigned int byte;
		int consumed = 0;
		if (sscanf(p, " %2x%n", &byte, &consumed) != 1){
			break;
		}
		out[n++] = (uint8_t)byte;
		p += consumed;
	}
	return n;
}

FILE *read_file(const char *filename){
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL){
	    return NULL;
    }
    return fp;
}

int get_file_line(FILE *fp, wp_frame_cb_t cb, void *user_data){
    if (fp == NULL || cb == NULL){
        return -1;
    }
    char line[1024];
    int frames = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r'){
            continue;
        }
        uint8_t buf[256];
        int len = hexline_to_bytes(line, buf, sizeof(buf));
        if (len <= 0){
            continue;
        }
        wp_frame_t frame = {0};
        frame.payload = buf;
        frame.payload_len = (size_t)len;
        frame.medium = WP_MEDIUM_REPLAY;
        cb(&frame, user_data);
        frames++;
    }
    if (ferror(fp)){
        return -1;
    }
    return frames;
}

int wp_replay_run(const char *path, wp_frame_cb_t cb, void *user_data){
    if (path == NULL || cb == NULL){
        return -1;
    }
    FILE *fp = read_file(path);
    if (fp == NULL){
        return -1;
    }
    int frames = get_file_line(fp, cb, user_data);
    fclose(fp);
    return frames;
}
