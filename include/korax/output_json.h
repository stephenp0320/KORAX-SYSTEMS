/* Serialises tracks and verdicts to the NDJSON wire format the daemon
 * broadcasts over the Unix-domain socket (see server.c). The JSON shape
 * produced here — field names, "validation" block layout, rule/severity
 * strings — is a public contract pinned byte-for-byte by the golden test
 * (test_golden_verdict). Do not change it casually; any deliberate change
 * must update the golden string in the same commit. */
#ifndef KORAX_OUTPUT_JSON_H
#define KORAX_OUTPUT_JSON_H
#include <stddef.h>
#include "korax/rook_track.h"
#include "korax/magpie_validator.h"

#define KX_JSON_SCHEMA_VERSION 1

/* Renders one track (position/velocity + validation block) as a single JSON
 * object into buffer. Returns the written length (excluding NUL) on success,
 * -1 on bad args/encoding failure, -2 if buffer is too small to hold the
 * result — callers must size buffers generously for findings (a track with
 * many findings can approach ~1KB) or updates are silently dropped. */
int track_to_json(const track_t *track, char *buffer, size_t len);

/* Renders just the {"validation":{...}} block for a verdict/findings array.
 * Same -1 (bad args/encoding) / -2 (buffer too small) return convention as
 * track_to_json. */
int verdict_to_json(kx_verdict_t *verdict, uint8_t count, char *buffer, size_t len);
#endif
