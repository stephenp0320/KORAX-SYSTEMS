<div align="center">

<img src="https://github.com/user-attachments/assets/cea40012-4af1-401f-9653-ee0a2e78612a"
     alt="KORAX Systems Logo"
     width="180"/>

# KORAX Systems

### High-Performance Remote ID Decoding, Validation & Tracking

*A modular C framework for decoding, validating and tracking ASTM F3411 & ASD-STAN Remote ID broadcasts over Bluetooth and Wi-Fi.*

<p align="center">
<img src="https://img.shields.io/badge/C-C11-blue.svg">
<img src="https://img.shields.io/badge/Build-Make-success.svg">
<img src="https://img.shields.io/badge/Status-Alpha-orange.svg">
<img src="https://img.shields.io/badge/Platform-macOS%20%7C%20Linux-lightgrey.svg">
<img src="https://img.shields.io/badge/License-Apache%202.0-green.svg">
</p>

<p align="center">
<a href="#overview">Overview</a> •
<a href="#features">Features</a> •
<a href="#architecture">Architecture</a> •
<a href="#building">Building</a> •
<a href="#roadmap">Roadmap</a> •
<a href="#contributing">Contributing</a>
</p>

</div>

---

# Overview

KORAX Systems is an open-core Remote ID processing engine designed for drone detection, compliance validation, and Counter-Unmanned Aircraft System (C-UAS) applications.

The framework captures Bluetooth Low Energy (BLE) and Wi-Fi Remote ID broadcasts, decodes the ASTM F3411 / ASD-STAN protocol into structured data, validates protocol compliance, and maintains live drone tracks that can be integrated into larger surveillance and airspace management systems.

Rather than building another complete drone detection platform, KORAX focuses on providing the software layer that existing platforms can embed to perform standards-compliant Remote ID processing.

---

# Why KORAX?

Modern drones are legally required to broadcast Remote ID messages, but many manufacturers implement the standard inconsistently.

KORAX exists to provide:

- Reliable protocol decoding
- Standards-compliant validation
- High-performance tracking
- A reusable software SDK
- Clean APIs for integration
- An open-core foundation for commercial systems

---

# Features

## Current

✔ Bluetooth Remote ID extraction

✔ Wi-Fi Remote ID extraction

✔ ASTM F3411 message decoding

✔ ASD-STAN message decoding

✔ Support for:

- Basic ID
- Location / Vector
- Authentication
- Self ID
- System
- Operator ID
- Message Pack

✔ Drone track management

✔ Modular processing pipeline

✔ Portable C11 implementation

---

## Planned

- Remote ID validator
- JSON serialization
- REST API
- MQTT publisher
- gRPC interface
- PCAP replay improvements
- Live Bluetooth capture
- Live Wi-Fi capture
- Sensor fusion
- Performance benchmarks
- Docker images
- Continuous Integration
- GitHub Packages
- Documentation website

---

# Architecture

```
                   Bluetooth
                        │
                        │
                  Wi-Fi Beacon
                        │
                        ▼
             Capture / Extraction
                        │
                        ▼
             Raven Decode Engine
                        │
                        ▼
            Message Processing Pipeline
                        │
                        ▼
             Rook Track Management
                        │
                        ▼
          Protocol Validation Engine
                        │
                        ▼
      JSON │ REST │ MQTT │ SDK │ CLI
```

---

# Project Structure

```
KORAX-SYSTEMS
│
├── include/
│   ├── capture/
│   ├── pipeline/
│   ├── raven/
│   ├── rook/
│   └── validator/
│
├── src/
│   ├── capture/
│   ├── pipeline/
│   ├── raven/
│   ├── rook/
│   └── validator/
│
├── examples/
│   ├── demo_decode.c
│   └── korax_replay.c
│
├── tests/
│
├── docs/
│
├── Makefile
│
└── README.md
```

---

# Components

## Raven

The protocol decoding engine.

Responsible for converting raw Bluetooth and Wi-Fi Remote ID payloads into strongly typed C structures.

---

## Capture

Extracts Remote ID payloads from supported wireless transports.

Supported transports include:

- Bluetooth Legacy Advertising
- Bluetooth Extended Advertising
- Bluetooth Long Range
- Wi-Fi Beacon Frames
- Wi-Fi NAN

---

## Pipeline

Coordinates the complete message flow.

```
Raw Packet
     │
     ▼
Payload Extraction
     │
     ▼
Decode
     │
     ▼
Track Update
     │
     ▼
Validation
     │
     ▼
Output
```

---

## Rook

The tracking engine.

Maintains active drone tracks by:

- Creating new tracks
- Updating existing tracks
- Correlating multiple message types
- Expiring inactive drones

---

## Validator *(In Development)*

The validator checks decoded messages against the Remote ID specification.

Examples include:

- Invalid coordinates
- Invalid altitude
- Invalid timestamps
- Missing mandatory messages
- Reserved value usage
- Invalid message lengths
- Authentication consistency
- Specification compliance

---

# Example Output

```
===== Basic ID =====

UA Type:
UATYPE_AEROPLANE

ID:
1581F5BJD248Q00A8XVX

===== Location =====

Latitude:
51.8979

Longitude:
-8.4706

Altitude:
120.5 m

Timestamp:
15.4 s
```

---

# Building

## Requirements

- GCC or Clang
- Make
- C11 compiler

---

Clone the repository

```bash
git clone https://github.com/<your-username>/KORAX-SYSTEMS.git

cd KORAX-SYSTEMS
```

Build

```bash
make
```

Run the decoder demo

```bash
./korax_demo
```

Replay captured frames

```bash
make replay
```

Run tests

```bash
make test
```

Clean

```bash
make clean
```

---

# Design Goals

KORAX has been designed around five core principles.

## Performance

Efficient C implementation suitable for embedded and real-time systems.

---

## Modularity

Each subsystem performs a single responsibility and communicates through clean interfaces.

---

## Standards Compliance

Implements ASTM F3411 and ASD-STAN Remote ID specifications.

---

## Portability

Runs on Linux, macOS and other POSIX-compatible platforms.

---

## Integration

Designed to be embedded inside larger surveillance, aviation and C-UAS systems.

---

# Intended Applications

KORAX can be integrated into:

- Counter-UAS platforms
- Airport surveillance systems
- Airspace monitoring software
- Drone manufacturer validation tools
- Drone compliance software
- Security monitoring systems
- Research projects
- Remote ID testing tools

---

# Roadmap

## Phase 1

- Complete decoder
- BLE extraction
- Wi-Fi extraction
- Tracking engine

---

## Phase 2

- Validation engine
- JSON export
- REST interface
- MQTT output
- Replay improvements

---

## Phase 3

- Sensor fusion
- Performance optimisation
- Docker deployment
- GitHub Packages
- Public SDK
- Documentation site

---

# Documentation

Additional documentation will include:

- Protocol specification
- Architecture guide
- API reference
- Validator design
- Track engine internals
- Performance benchmarks

---

# Contributing

Contributions are welcome.

Please ensure that all pull requests:

- follow the existing coding style
- include appropriate tests
- update documentation where necessary

---

# License

Licensed under the Apache License 2.0.

See the LICENSE file for details.

---

# Author

**Stephen Power**

Software Engineer

Drone Systems • Embedded Software • Protocol Engineering

---

# Vision

KORAX aims to become the reference open-source Remote ID decoding and validation framework for the European drone ecosystem.

By providing a high-performance, standards-compliant software foundation, KORAX enables manufacturers, system integrators and airspace-security platforms to build trusted drone detection solutions without reimplementing complex Remote ID protocols from scratch.

---

<div align="center">

Made with ❤️ in Ireland 🇮🇪

</div>
