# Contributing to Kernel

Thank you for your interest in contributing to the Pipeline of Coherence framework.

## Getting Started

```bash
git clone https://github.com/Quigles1337/kernels-refactor.git
cd kernels-refactor
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
cd build && ctest --output-on-failure
```

## Architecture Rules

### Constants — Single Source of Truth

All mathematical constants are defined in `include/kernel/core/constants.hpp` and **nowhere else**. If you need a constant that doesn't exist, add it there. Never define local copies with prefixes like `MY_ETA` or `LOCAL_PI`.

```cpp
// WRONG — do not do this
constexpr double MY_ETA = 0.70710678118654752440;

// RIGHT — use the shared constant
#include <kernel/core/constants.hpp>
double x = kernel::ETA;
```

### Theorem Functions — Single Implementation

All theorem-derived functions (`coherence()`, `lyapunov()`, `palindrome_residual()`, etc.) are implemented once in `include/kernel/core/theorems.hpp`. Do not re-implement these in other headers or tests.

### Namespaces

Every header places its contents in exactly one namespace:

| Namespace | Contents |
|-----------|----------|
| `kernel::` | Constants, types, theorem functions |
| `kernel::scheduling::` | QuantumKernel, Process, Memory, Periodicity |
| `kernel::scheduling::ipc::` | QuantumIPC, Message, Channel |
| `kernel::scheduling::interrupts::` | DecoherenceHandler |
| `kernel::pipeline::` | KernelState, SpectralBridge, Pipeline |
| `kernel::quantum::` | ChiralNonlinearGate, PalindromePrecession, LadderSearch |
| `kernel::oracle::` | MasterEigenOracle |
| `kernel::ohm::` | Ohm–Coherence Duality models |
| `kernel::qudit::` | Qudit extension |

### Header Guards

Use `#pragma once` in all headers.

### Include Paths

Use angle-bracket includes with the `kernel/` prefix:

```cpp
#include <kernel/core/constants.hpp>
#include <kernel/kernel/quantum_kernel.hpp>
```

## Testing

- **Unit tests** go in `tests/unit/` — test a single component in isolation
- **Integration tests** go in `tests/integration/` — test interactions between components
- All tests must pass before merging: `cd build && ctest --output-on-failure`

## Formal Verification

If you modify a theorem function, verify the Lean 4 proofs still build:

```bash
cd formal/lean && lake build
```

## Submitting Changes

1. Fork the repository
2. Create a feature branch from `main`
3. Make your changes, ensuring all tests pass
4. Submit a pull request with a clear description of the change

## Code Style

- C++17 standard
- 2-space indentation for code, 4-space for continued lines
- Document mathematical theorems with their reference number from `docs/master_derivations.pdf`
- Keep headers self-contained (each header compiles independently)
