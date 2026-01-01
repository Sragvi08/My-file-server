# Secure TCP Expression Server

## Overview

This project implements a **secure, concurrent TCP client–server application in C** that evaluates arithmetic expressions sent by clients. The server is designed with **defensive programming principles**: strict input validation, bounded memory usage, controlled concurrency, and graceful shutdown. Any malformed input or unsafe operation deterministically returns **`0.000`**, ensuring the system fails closed.

The implementation emphasizes **systems security fundamentals** relevant to cybersecurity roles, including resource management, untrusted input handling, and concurrency safety.

---

## Key Features

- **Concurrent server** using POSIX sockets and pthreads
- **Fixed memory allocation per worker** to prevent exhaustion
- **Strict expression parsing** with PEMDAS precedence (including right-associative exponentiation)
- **Fail-closed error handling** (malformed input → `0.000`)
- **Division-by-zero and overflow protection**
- **Connection limits and time-bound execution**
- **Graceful shutdown** with thread joins and resource cleanup

---

## Supported Operations

- Addition (`+`)
- Subtraction (`-`)
- Multiplication (`*`)
- Division (`/`) — division by zero returns `0.000`
- Exponentiation (`^`) — non-finite results return `0.000`

Only **positive numeric operands** are accepted. Any deviation from the expected grammar is treated as an error.

---

## Usage

### Server Mode

```bash
myfs 0 <port>
