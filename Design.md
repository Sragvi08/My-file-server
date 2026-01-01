## DESIGN.md

### 1. Design Goals

* Safely process **untrusted network input**
* Prevent **resource exhaustion** (CPU, memory, threads)
* Ensure **deterministic behavior on error**
* Maintain **clear thread lifecycle management**
* Adhere strictly to project‑specified constraints

---

### 2. Threat Model

The server assumes clients may be:

* Malicious or buggy
* Sending malformed or adversarial input
* Attempting resource exhaustion (DoS‑style behavior)

**Out of scope**:

* Man‑in‑the‑middle attacks
* Credential compromise
* Cryptographic attacks

---

### 3. Defensive Measures

#### Input Validation

* Expressions are parsed manually using `sscanf` with `%n` to track consumption
* All remaining characters are validated
* Any parsing inconsistency marks the transaction as invalid

#### Arithmetic Safety

* Division by zero explicitly checked
* Exponentiation validated with `isfinite()`
* Any unsafe arithmetic returns `0.000`

#### Memory Safety

* Each worker thread performs **exactly one bounded allocation**
* No dynamic resizing or unbounded buffers
* Worker returns results via memory it owns, reclaimed by the master

#### Concurrency Control

* Hard cap on concurrent worker threads (`MAXT`)
* Excess clients are rejected gracefully
* Threads are **joined**, not detached, enabling cleanup and aggregation

#### Availability Protection

* Server operates under a strict global time limit
* Non‑blocking accept loop prevents server lockup

---

### 4. Thread Lifecycle

1. Master accepts client connection
2. Worker thread created with dedicated socket
3. Worker processes client input line‑by‑line
4. Worker accumulates local result
5. Worker exits and returns result pointer
6. Master thread joins worker and aggregates totals

This design avoids shared mutable global state.

---

### 5. Failure Handling Philosophy

The system is designed to **fail closed**:

* Errors do not propagate
* Invalid input does not crash the server
* Output is always well‑formed

Returning `0.000` on error ensures:

* Predictable behavior
* No information leakage
* Compliance with project requirements

---

### 6. Testing Strategy

* Valid arithmetic expressions
* Malformed expressions
* Division by zero cases
* Large input files
* Concurrent client connections

Testing focuses on **robustness rather than performance tuning**.

---

### 7. Security Takeaways

This project demonstrates:

* Defensive C programming
* Secure handling of untrusted input
* Thread‑safe server design
* Explicit resource lifecycle management

These principles directly map to **systems security and backend security engineering roles**.
