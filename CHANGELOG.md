# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project aims
to follow [Semantic Versioning](https://semver.org/) from 1.0.

## [Unreleased]

### Added
- Professional project layout: namespaced public headers under `include/korax/`,
  CMake build with `libkorax` static library and CTest integration, CI workflow,
  and open-source meta files (LICENSE, NOTICE, CONTRIBUTING, SECURITY).
- `prev_location` tracking and a haversine jump/speed check (initial).

### Changed
- Error handling now uses a granular `decode_result_t` enum instead of a single
  error code.

### Fixed
- Full ODID wire-format accuracy across all six message types and the message
  pack (endianness, flag-bit packing, field scaling).

## [0.1.0] - unreleased
- Initial decode engine and in-memory track table.
