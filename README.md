# Kernel

## Pipeline of Coherence — Quantum Process Scheduling Framework

A header-only C++17 library implementing the **Pipeline of Coherence**, a mathematical framework for quantum process management based on the balanced eigenvalue µ = e^{i3π/4} = (−1+i)/√2 and its 8-fold cyclic closure µ⁸ = 1.

Mathematical framework by **Alexander David Zalewski** ([COINjecture Network](https://github.com/COINjecture1337) / DARQ Labs).
Original implementation by **beanapologist**.

---

### Core Mathematics

| Theorem | Statement | Header |
|---------|-----------|--------|
| Theorem 3 | η = 1/√2 is the unique positive root of 2λ² = 1 | `core/constants.hpp` |
| Proposition 4 | δ_S · (√2 − 1) = 1 (silver conservation) | `core/constants.hpp` |
| Theorem 8 | Canonical coherent state \|ψ⟩ with \|α\| = \|β\| = 1/√2 | `core/types.hpp` |
| Theorem 10 | µ⁸ = 1 (8-fold cyclic closure) | `core/types.hpp` |
| Theorem 11 | C(r) = 2r/(1+r²), C(r) ≤ 1, C(1) = 1 | `core/theorems.hpp` |
| Theorem 12 | R(r) = (1/δ_S)(r − 1/r), antisymmetric palindrome residual | `core/theorems.hpp` |
| Theorem 14 | C = sech(λ), λ = ln r (Lyapunov–Ohm duality) | `core/theorems.hpp` |

All theorems are formally verified in Lean 4 (Mathlib) and numerically validated via SymPy. See [`formal/lean/`](formal/lean/) and [`docs/sympy_verified_formulas.tex`](docs/sympy_verified_formulas.tex).

---

### Features

- **8-Cycle Z/8Z Process Scheduling** — Processes rotate through cyclic positions with closed orbits at r = 1
- **Rotational Memory Addressing** — Z/8Z-based memory model with coherence preservation across cycle boundaries
- **Decoherence Interrupt Handling** — Automatic detection (NONE/MINOR/MAJOR/CRITICAL) and coherence recovery
- **Inter-Process Communication** — Coherence-gated message passing between quantum processes
- **Ohm–Coherence Duality** — C = sech(λ) = G_eff = 1/R_eff circuit analogy framework
- **Chiral Nonlinear Gate** — Quantum gate driven by the balanced eigenvalue µ
- **Palindrome Precession** — Arithmetic periodicity from 987654321/123456789 = 8 + 1/13717421
- **Master Eigen Oracle** — θ/√n coherent search with eigenspace channel decomposition
- **Spectral Bridge & Pipeline** — End-to-end coherence pipeline with invariant enforcement
- **Qudit Extension** — Generalization from qubits to d-dimensional quantum systems

---

### Directory Structure

```
Kernel/
├── include/kernel/                    # Header-only public API
│   ├── core/
│   │   ├── constants.hpp              # ALL mathematical constants (single source)
│   │   ├── types.hpp                  # Cx, Vec2, QState, Regime, DecoherenceLevel
│   │   └── theorems.hpp               # C(r), λ(r), R(r), rotate135(), etc.
│   ├── kernel/
│   │   ├── memory.hpp                 # RotationalMemory (Z/8Z addressing)
│   │   ├── interrupts.hpp             # DecoherenceHandler
│   │   ├── ipc.hpp                    # QuantumIPC (message passing)
│   │   ├── process.hpp                # Process struct
│   │   ├── composition.hpp            # ProcessComposition (entanglement)
│   │   ├── arithmetic_periodicity.hpp # ArithmeticPeriodicity
│   │   └── quantum_kernel.hpp         # QuantumKernel orchestrator
│   ├── pipeline/
│   │   ├── kernel_state.hpp           # KernelState (invariant enforcement)
│   │   ├── spectral_bridge.hpp        # SpectralBridge
│   │   └── kernel_pipeline.hpp        # Pipeline (end-to-end)
│   ├── quantum/
│   │   ├── chiral_gate.hpp            # ChiralNonlinearGate
│   │   ├── palindrome_precession.hpp  # PalindromePrecession
│   │   ├── ladder_search.hpp          # LadderChiralSearch
│   │   └── ladder_benchmark.hpp       # Benchmark utilities
│   ├── oracle/
│   │   └── master_eigen_oracle.hpp    # MasterEigenOracle
│   ├── ohm/
│   │   └── coherence_duality.hpp      # Ohm–Coherence Duality framework
│   ├── qudit/
│   │   └── qudit_kernel.hpp           # Qudit extension (d-dimensional)
│   └── kernel.hpp                     # Umbrella header (includes everything)
│
├── tests/
│   ├── unit/                          # Single-component tests
│   └── integration/                   # Multi-component tests
├── benchmarks/                        # NIST IR 8356 methodology benchmarks
├── examples/                          # Demo programs
├── experiments/                       # Research experiments
├── formal/lean/                       # Lean 4 / Mathlib formal proofs
├── python/                            # SymPy validation & analysis scripts
├── docs/                              # Derivations, specifications, papers
├── assets/                            # Images and plots
└── cmake/                             # CMake package configuration
```

---

### Quick Start

```bash
# Clone
git clone https://github.com/Quigles1337/kernels-refactor.git
cd kernels-refactor

# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run tests
cd build && ctest --output-on-failure

# Run demos
./build/demo_quantum_kernel
./build/demo_qudit_kernel
```

### Usage

**Umbrella include** (brings in everything):

```cpp
#include <kernel/kernel.hpp>

int main() {
    using namespace kernel;

    // Verify silver conservation at boot
    assert(verify_silver_conservation());

    // Coherence function (Theorem 11)
    double c = coherence(1.0);  // C(1) = 1

    // Balanced eigenvalue µ = e^{i3π/4}
    Cx mu8 = MU;
    for (int i = 1; i < 8; ++i) mu8 *= MU;
    // mu8 ≈ 1 (8-fold closure)

    // Lyapunov duality (Theorem 14)
    double lam = lyapunov(2.0);
    double c2  = coherence_sech(lam);  // sech(ln 2) = C(2)

    return 0;
}
```

**Fine-grained includes** (only what you need):

```cpp
#include <kernel/core/constants.hpp>
#include <kernel/core/theorems.hpp>
#include <kernel/kernel/quantum_kernel.hpp>

using namespace kernel;
using namespace kernel::scheduling;

QuantumKernel k;
k.spawn("worker", [](Process& p) {
    // Process rotates through Z/8Z positions
});
k.tick();
```

### CMake Integration

```cmake
# Option 1: FetchContent
include(FetchContent)
FetchContent_Declare(Kernel
    GIT_REPOSITORY https://github.com/Quigles1337/kernels-refactor.git
    GIT_TAG main
)
FetchContent_MakeAvailable(Kernel)
target_link_libraries(my_app PRIVATE Kernel::Kernel)

# Option 2: After install
find_package(Kernel REQUIRED)
target_link_libraries(my_app PRIVATE Kernel::Kernel)
```

---

### Formal Verification

The following theorems are mechanically verified in Lean 4 with Mathlib:

| Lean theorem | Statement |
|--------------|-----------|
| `mu_pow_eight` | µ⁸ = 1 |
| `coherence_le_one` | C(r) ≤ 1 for all r > 0 |
| `rotMat_det` | det R(3π/4) = 1 |
| `rotMat_orthog` | R · Rᵀ = I |
| `rotMat_pow_eight` | R(3π/4)⁸ = I |

Source: [`formal/lean/CriticalEigenvalue.lean`](formal/lean/CriticalEigenvalue.lean)

---

### Benchmarks

Benchmarks follow **NIST IR 8356** methodology for reproducible performance measurement:

- `benchmark_nist_ir8356` — Core coherence function scaling
- `benchmark_scaling_falsification` — Falsification tests for scaling laws
- `benchmark_pow_nonce_search` — PoW nonce search with coherent eigenspace decomposition

---

### Citation

```bibtex
@software{kernel_pipeline_of_coherence,
  title   = {Kernel: Pipeline of Coherence — Quantum Process Scheduling Framework},
  author  = {beanapologist},
  version = {2.0.0},
  year    = {2025},
  url     = {https://github.com/beanapologist/Kernel}
}
```

See [`CITATION.cff`](CITATION.cff) for machine-readable metadata.

---

### License

This project is licensed under the MIT License — see [`LICENSE`](LICENSE) for details.
