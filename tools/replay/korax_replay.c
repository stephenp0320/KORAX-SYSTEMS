#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "korax/raven_decode.h"
#include "korax/rook_track.h"
#include "korax/pipeline.h"
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

static void handle_frame(const uint8_t *buf, int len, track_table_t *table){
    uint8_t type = (buf[0] >> 4) & 0x0F;
    if (type == MESSAGETYPE_PACKED) {
        decode_message_pack((uint8_t *)buf, len, table);
    } else {
        decoded_message_t msg;
        if (decode_message(buf, len, &msg) == DECODE_OK)
            process_decoded_message(table, &msg);
    }
}

int main(int argc, char **argv){
	const char *filename = (argc > 1) ? argv[1] : "frames.txt";
	FILE *fp;
	fp = fopen(filename, "r");
	if (fp == NULL){
		fprintf(stderr, "korax_replay: cannot open '%s'\n", filename);
		return 1;
	}
	track_table_t table;
	track_table_init(&table);

	char line[1024];
    	int frames = 0;
    	while (fgets(line, sizeof(line), fp)) {
        	if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
			continue;
		uint8_t buf[256];
		int len = hexline_to_bytes(line, buf, sizeof(buf));
		if (len == 0)
			continue;
		handle_frame(buf, len, &table);
		frames++;
	}
	fclose(fp);
 
    	printf("Replayed %d frame(s) from %s\n", frames, filename);
    	printf("Tracks seen: %d\n\n", table.num_tracks);
 
    	for (int i = 0; i < table.num_tracks; i++) {
        	track_t *t = &table.tracks[i];
        	printf("Drone %d\n", i + 1);
        	printf("  UAS ID     : %s\n", t->uas_id);
        	printf("  Operator   : %s\n", t->has_operator_id ? t->operator_id.OperatorId : "(none)");
        	printf("  Position   : %.5f, %.5f\n", t->location.Latitude, t->location.Longitude);
        	printf("  Altitude   : %.1f m (geodetic)\n", t->location.AltitudeGeo);
        	printf("  Speed/Head : %.1f m/s @ %.0f deg\n", t->location.SpeedHorizontal, t->location.Direction);
        	printf("  Messages   : basic=%d location=%d system=%d operator=%d self=%d\n",
               	t->has_basic_id, t->has_location, t->has_system,
         	t->has_operator_id, t->has_self_id);
        	printf("\n");
    	}
    	return 0;
}
