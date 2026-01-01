# Secure TCP Expression Server

## README.md

### Overview

This project implements a **secure, concurrent TCP client–server application in C** that evaluates arithmetic expressions sent by clients. The server is designed with **defensive programming principles**: strict input validation, bounded memory usage, controlled concurrency, and graceful shutdown. Any malformed input or unsafe operation deterministically returns **`0.000`**, ensuring the system fails closed.

The implementation emphasizes **systems security fundamentals** relevant to cybersecurity roles, including resource management, untrusted input handling, and concurrency safety.

---

### Key Features

* **Concurrent server** using POSIX sockets and pthreads
* **Fixed memory allocation per worker** to prevent exhaustion
* **Strict expression parsing** with PEMDAS precedence (including right‑associative exponentiation)
* **Fail‑closed error handling** (malformed input → `0.000`)
* **Division‑by‑zero and overflow protection**
* **Connection limits and time‑bound execution**
* **Graceful shutdown** with thread joins and resource cleanup

---

### Supported Operations

* Addition (`+`)
* Subtraction (`-`)
* Multiplication (`*`)
* Division (`/`) — division by zero returns `0.000`
* Exponentiation (`^`) — non‑finite results return `0.000`

Only **positive numeric operands** are accepted. Any deviation from the expected grammar is treated as an error.

---

### Usage

#### Server Mode

```bash
myfs 0 <port>
```

#### Client Mode

```bash
myfs 1 <host> <port> < infile > outfile
```

Each line in the input file is treated as a separate arithmetic expression. The output appends the computed result (formatted to three decimal places) to each input line.

---

### Output Semantics

* **Valid expression** → computed result
* **Invalid expression / unsafe operation** → `0.000`

Examples of invalid input include:

* Malformed syntax
* Extra tokens or characters
* Division by zero
* Numeric overflow or non‑finite exponentiation

---

### Security Considerations

* Enforces a maximum number of concurrent clients (`MAXT`)
* Uses fixed‑size buffers to prevent memory overuse
* Applies a global server time limit (`TIMELIMIT`)
* Treats all client input as untrusted
* Avoids global mutable state in worker threads
* Ensures all resources (threads, sockets, memory) are reclaimed

---

### Build & Environment

* Language: **C (POSIX)**
* Platform: Linux / Unix‑like systems
* Libraries: `pthread`, `math`, standard socket APIs

---

### Limitations

* No encryption (TLS not implemented)
* No authentication or authorization
* IPv4 only

These limitations are acknowledged and documented as part of the security design.

---

