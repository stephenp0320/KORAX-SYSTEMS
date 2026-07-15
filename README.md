# Korax

**Remote ID decode, track and validation engine.**

[![CI](https://github.com/stephenp0320/KORAX-SYSTEMS/actions/workflows/ci.yml/badge.svg)](https://github.com/stephenp0320/KORAX-SYSTEMS/actions/workflows/ci.yml)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](LICENSE)
[![Standard](https://img.shields.io/badge/spec-ASTM%20F3411-informational)](https://www.astm.org/f3411-22a.html)

Korax is a small, embeddable C library that listens to the **Remote ID** signal
almost every drone is now legally required to broadcast, decodes it into
structured tracks, and validates whether each broadcast can be trusted. It is
built to be embedded inside counter-drone platforms, airspace-security systems,
and manufacturer compliance tooling rather than run as a standalone appliance.

> **Status:** pre-alpha (v0.1). The decode engine is complete and verified
> against the ASTM F3411 / opendroneid wire format; capture and the validation
> engine are in progress. See the [roadmap](#roadmap).

## Why

Remote ID is a mandatory "digital licence plate" — a drone's ID, position,
altitude, velocity and operator location, broadcast over Bluetooth and Wi-Fi
under ASTM F3411 and ASD-STAN prEN 4709-002. The standard is enforced largely
by manufacturer self-certification, and real-world broadcasts vary widely in
quality. Every downstream security system therefore needs the same two things:
to decode the signal **correctly**, and to know when it **cannot be trusted**.
Korax provides both as a component you embed, not a platform you buy.

## Architecture

The pipeline runs left to right; each stage is independently testable.

```
capture → Raven (decode) → Rook (track) → Magpie (validate) → output
```

| Module  | Directory        | Responsibility                                   |
| ------- | ---------------- | ------------------------------------------------ |
| Capture | `src/capture`    | Get Remote ID bytes off BLE / Wi-Fi frames       |
| Raven   | `src/raven`      | Decode ODID messages into typed structs          |
| Rook    | `src/rook`       | Maintain the live table of drone tracks          |
| Magpie  | `src/magpie`     | Flag non-conformant / untrustworthy broadcasts   |
| Pipeline| `src/pipeline`   | Wire decode → track together                      |

Public headers live under [`include/korax/`](include/korax); consumers
`#include "korax/raven_decode.h"`.

## Build

Korax builds with CMake (recommended) or a plain Makefile.

### CMake

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

This produces the static library `libkorax.a` plus the tools, examples and
tests. Options: `-DKORAX_BUILD_TESTS`, `-DKORAX_BUILD_TOOLS`,
`-DKORAX_BUILD_EXAMPLES`, `-DKORAX_WERROR`.

### Make

```sh
make test      # build + run the unit tests
make replay    # build + run the replay tool against a sample capture
```

## Quickstart — replay a drone with no hardware

`korax_replay` feeds a file of Remote ID frames through the full
decode → track pipeline, exactly as a live radio would:

```sh
make replay
# or:
./build/korax_replay tools/replay/frames.txt
```

You will see one tracked drone with its decoded ID, position, altitude and
speed. The sample [`frames.txt`](tools/replay/frames.txt) is a synthetic but
spec-correct capture of one multirotor taking off, climbing and cruising.

## Repository layout

```
include/korax/   public API headers
src/             library modules (raven, rook, capture, magpie, pipeline)
tools/replay/    korax_replay + sample capture
examples/        small usage examples
tests/unit/      unit tests (run via ctest or `make test`)
docs/            verification notes and design docs
```

## Roadmap

- [x] Wire-accurate ODID decoder (all message types + message pack)
- [x] In-memory track table with expiry
- [ ] Live BLE / Wi-Fi capture (Linux)
- [ ] Magpie validation engine
- [ ] JSON / socket output daemon
- [ ] Sensor fusion

## Contributing

Contributions are welcome under the [DCO](CONTRIBUTING.md). Please read
[CONTRIBUTING.md](CONTRIBUTING.md) and [SECURITY.md](SECURITY.md).

## License

Apache-2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).

Korax is an independent implementation of a public standard and is not
affiliated with ASTM, ASD-STAN, or the opendroneid project.
