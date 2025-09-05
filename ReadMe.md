# C++20 Embedded Networking Stack

> From‑scratch implementation of a portable TCP/IP networking stack, built with modern C++20 and designed with the constraints of embedded systems in mind.

<p align="center">
  <a href="https://en.cppreference.com/w/cpp/compiler_support/20"><img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-blue.svg"></a>
  <img alt="No dynamic allocation" src="https://img.shields.io/badge/heap-none-critical.svg">
  <a href="https://cmake.org/"><img alt="CMake" src="https://img.shields.io/badge/build-cmake-1f425f.svg"></a>
  <img alt="Platform" src="https://img.shields.io/badge/platform-embedded%20%7C%20windows-lightgrey.svg">
  <img alt="Status" src="https://img.shields.io/badge/status-experimental-orange.svg">
</p>

---

## Table of Contents

* [Project Goal](#project-goal)
* [Core Philosophy & Guiding Principles](#core-philosophy--guiding-principles)
* [Current Status](#current-status)
* [Project Architecture](#project-architecture)
* [Directory Layout](#directory-layout)
* [Getting Started (Windows PC)](#getting-started-windows-pc)

  * [Prerequisites](#prerequisites)
  * [Build](#build)
  * [Run](#run)
* [Porting to New Hardware (HAL)](#porting-to-new-hardware-hal)
* [Design Highlights (C++20)](#design-highlights-c20)
* [Roadmap](#roadmap)
* [Motivation](#motivation)
* [Contributing](#contributing)
* [License](#license)

---

## Project Goal

The primary goal of this project is to **demystify network programming** by building a complete, standards‑compliant networking stack layer by layer. This is both a deep dive into the fundamentals of protocols like **Ethernet, ARP, IP, UDP, and TCP**, and a practical exploration of **modern C++20** features for creating safe, efficient, and portable systems code.

The final library is intended to be **portable enough to run on a bare‑metal microcontroller** with a dedicated Ethernet chip (e.g., **W5500**), while also supporting **PC development and testing** via an **Npcap‑based HAL**.

---

## Core Philosophy & Guiding Principles

This stack is being built with a specific set of architectural principles, mirroring best practices for professional embedded firmware development.

* **Portability First (Hardware Abstraction Layer — HAL)**
  The core stack logic (`net_stack/`) is completely **decoupled** from the underlying hardware and operating system. It communicates with the physical world through a **clean set of interfaces** defined in the `hal/` directory. To port this stack to a new microcontroller, implement the HAL for that target.

* **Embedded‑Focused (No Dynamic Allocation)**
  The stack is designed to be **statically allocated**. It avoids `new`, `malloc`, `std::vector`, and any other mechanisms that rely on a heap. All packet buffers and data structures use **compile‑time fixed‑size `std::array`** to ensure predictable memory usage and eliminate the risk of fragmentation and allocation failures common in long‑running embedded applications.

* **Modern C++20 for Safety & Expressiveness**
  We leverage modern C++ features as **zero‑cost abstractions** to write safer, more readable, and more robust code than a traditional C implementation.

  * Namespaces (`net::`) to prevent naming collisions.
  * `std::span` and `std::byte` for safe, bounds‑aware buffer manipulation.
  * `std::optional` to safely handle return values that may not exist, avoiding null pointer errors.
  * `constexpr` to enforce compile‑time checks and constants.
  * Scoped `enum class` for type‑safe state management.

* **Layer‑by‑Layer Construction**
  The stack is built from the ground up, starting at **Layer 2**, to ensure a solid understanding of each protocol before building the next one on top.

---

## Current Status

The networking stack is currently **functional up to Layer 2.5**.

* [x] **Layer 2 (Ethernet)** — Construct and parse Ethernet II headers.
* [x] **Layer 2.5 (ARP)** — Complete, bidirectional ARP implementation:

  * Send ARP **who‑has** requests to resolve an IP to a MAC address.
  * Correctly parse incoming ARP packets (both requests and replies).
  * Maintain an **ARP cache** to store learned mappings.
  * Automatically send an **ARP reply** when receiving a request for its own IP address.
  * **Cache aging** mechanism to expire stale entries.

---

## Project Architecture

The codebase is organized into three main directories:

* **`/hal`** — *Hardware Abstraction Layer*. Contains the platform‑specific driver code and the public interfaces (`hal_*.h`) used by the portable core.
* **`/protocols`** — *On‑the‑wire data blueprints*. Simple, packed C++ structs that exactly match the binary layout of network protocol headers.
* **`/net_stack`** — *Portable core logic*. Implements the state machines and processing for the networking protocols.

### Directory Layout

```
repo/
├── hal/                # HAL interfaces + platform-specific impls (e.g., pc_npcap/, mcu_w5500/)
├── protocols/          # Packed structs for Ethernet, ARP, (IPv4, ICMP, UDP, TCP planned)
├── net_stack/          # Core protocol logic (portable, no OS/driver deps)
├── cmake/              # Toolchain files / helpers (optional)
├── CMakeLists.txt
└── README.md
```

---

## Getting Started (Windows PC)

### Prerequisites

* A **C++20** compliant compiler (MSVC, Clang, or GCC)
* **CMake** 3.20+
* **Npcap SDK** (developer SDK). Download from the Npcap website.

> **Note:** This repository uses a **pc‑Npcap HAL** for development and testing on Windows.

### Build

```bash
# 1) Clone
git clone https://github.com/<you>/<repo>.git
cd <repo>

# 2) Configure & build
mkdir build && cd build
cmake ..
cmake --build .
```

### Run

On success, the build produces an executable, e.g. on MSVC:

```
build/Debug/Networking.exe
```

(Use the `Release` configuration for optimized builds.)

---

## Porting to New Hardware (HAL)

To run on a microcontroller (e.g., with a **W5500** Ethernet chip), implement the minimal HAL contract and wire it up in your target build.


The core stack in `net_stack/` consumes only these interfaces. **No heap** usage is required; buffer sizes are **compile‑time constants**.

---

## Design Highlights (C++20)

* **`std::span` + `std::byte`** for safe buffer slicing and explicit byte semantics
* **`std::optional`** for fallible operations (e.g., ARP cache lookup)
* **`constexpr`** layout checks and constants for protocol fields
* **`enum class`** for protocol/type/state IDs to avoid implicit conversions
* **`[[nodiscard]]`** on functions where ignoring results is a bug

---

## Roadmap

* [ ] **Layer 3: IPv4 & ICMPv4**

  * [ ] Implement IPv4 packet construction and parsing
  * [ ] Implement IP header checksum algorithm
  * [ ] Implement ICMPv4 to respond to ping requests
* [ ] **Layer 4: UDP**

  * [ ] Implement UDP datagram construction and parsing
* [ ] **Layer 7: DHCP**

  * [ ] Implement a DHCP client to automatically acquire an IP address
* [ ] **Layer 4: TCP**

  * [ ] Implement the TCP state machine (three‑way handshake, etc.)
  * [ ] Implement reliable, ordered data transfer
* [ ] **Future Refactoring**

  * [ ] Refactor the buffer management system

---

## Motivation

I always wanted to learn network programming and, after looking at libraries like **LwIP** and **ThreadX Networking**, I decided to create something simpler but not less in functionality to use in my projects and also learn along the way.

---


