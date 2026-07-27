#ifndef KORAX_OUTPUT_JSON_H
#define KORAX_OUTPUT_JSON_H
#include <stddef.h>
#include "korax/rook_track.h"
#include "korax/magpie_validator.h"

#define KX_JSON_SCHEMA_VERSION 1
int track_to_json(const track_t *track, char *buffer, size_t len);
int verdict_to_json(kx_verdict_t *verdict, uint8_t count, char *buffer, size_t len);
#endif
