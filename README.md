# doip-server

[![CI/CD Pipeline](https://github.com/Magolves/doip-server/actions/workflows/ci.yml/badge.svg)](https://github.com/Magolves/doip-server/actions/workflows/ci.yml)
[![Coverage](https://raw.githubusercontent.com/Magolves/doip-server/badges/coverage-badge.svg)](https://github.com/Magolves/doip-server/actions/workflows/badge.yml)
[![License](https://img.shields.io/github/license/Magolves/doip-server)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.15+-blue.svg)](https://cmake.org/)
[![doctest](https://img.shields.io/badge/Tests-doctest-green.svg)](https://github.com/doctest/doctest)

C/C++ server library for ISO 13400-2 Diagnostics over IP (DoIP).

**CAUTION** The current API is under construction any may change at any time.

## Dependencies

`doip-server` uses `spdlog` for logging and `CLI11` for command line parsing. The lib is downloaded automatically. Or you may install it locally via

```bash
sudo apt install libspdlog-dev libcli11-dev
# For unit tests
sudo apt install doctest-dev
# For coverage tests
sudo apt install gcovr
```

See [Logging](./doc/Logging.md) for details.

### Getting started

Quick start — read the generated tutorial for the example server:

- Online (published): [https://magolves.github.io/doip-server/index.html](https://magolves.github.io/doip-server/index.html)
- Local example page: see `doc/DoIPServer.md` (included in the Doxygen HTML under "Example DoIP Server Tutorial").
- Example tutorial (direct): [https://magolves.github.io/doip-server/md_doc_DoIPServer.html](https://magolves.github.io/doip-server/md_doc_DoIPServer.html)

If you want to generate the docs locally, install Doxygen and Graphviz and
run:

```bash
# Install Doxygen and Graphviz
sudo apt install doxygen graphviz
doxygen Doxyfile
xdg-open docs/html/index.html
```

### Installation

1. To install the library on the system, first get the source files with:

```bash
git clone https://github.com/Magolves/doip-server.git
```

1. Enter the directory 'doip-server' and build the library with:

```bash
cmake . -Bbuild
cd build
make
```

1. To install the library into `/usr/lib/doip-server` use:

```bash
sudo make install
```

### Installing doctest

```bash
sudo apt install doctest
```

## Debugging

### Dump UDP

```bash
sudo tcpdump -i any udp port 13400 -X
```

## Examples

The project includes a small example DoIP server demonstrating how to
use the `DoIPServer` and `DoIPServerModel` APIs and how to register UDS
handlers.

- Example source files: `examples/discover/DoIPServer.cpp`,
  `examples/discover/DoIPServerModel.h`

See the "Examples" section in the generated Doxygen main page for
additional annotated links to these files.

## Acknowledgments

This project was initially inspired by and based on
[https://github.com/AVL-DiTEST-DiagDev/libdoip](https://github.com/AVL-DiTEST-DiagDev/doip-server) by AndiAn94 and GerritRiesch94.
Over time, the codebase has been substantially rewritten and evolved in a different (server) direction. The original license (Apache 2.0) was therefore kept.

The [original fork](https://github.com/Magolves/libdoip) is no longer maintained.

## References

- [ISO 13400-2:2019(en) Road vehicles — Diagnostic communication over Internet Protocol (DoIP) — Part 2: Transport protocol and network layer services](<https://www.iso.org/obp/ui/#iso:std:iso:13400:-2:ed-2:v1:en>)
- [Specification of Diagnostic over IP](<https://www.autosar.org/fileadmin/standards/R20-11/CP/AUTOSAR_SWS_DiagnosticOverIP.pdf>)
- [Diagnostics over Internet Protocol (DoIP)](<https://cdn.vector.com/cms/content/know-how/_application-notes/AN-IND-1-026_DoIP_in_CANoe.pdf>)
- [Diagnostics Over Internet Protocol (DoIP) in CANoe](<https://cdn.vector.com/cms/content/know-how/_application-notes/AN-IND-1-026_DoIP_in_CANoe.pdf>)
