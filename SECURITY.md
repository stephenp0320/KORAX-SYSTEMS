# Security Policy

## Reporting a vulnerability

Please report security issues privately rather than opening a public issue.
Email the maintainer at **stephenpower03@gmail.com** with details and, if
possible, a minimal reproducer. You will receive an acknowledgement, and we
ask that you allow a reasonable period for a fix before public disclosure.

## Threat model

Korax's entire input surface is unauthenticated radio broadcasts from unknown
parties — every frame is treated as hostile. Decode functions are designed to
be total over arbitrary byte input: any input produces either a typed result or
a specific error, never a crash or undefined behaviour. The decoders and the
message-pack unpacker are intended to be continuously fuzzed.

## Supported versions

Korax is pre-1.0; only the latest `main` is supported while the API stabilises.
