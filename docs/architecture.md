# Architecture

## Module Dependency Graph

```
kernel/core/constants.hpp          (no dependencies)
        │
kernel/core/types.hpp              (depends on: constants)
        │
kernel/core/theorems.hpp           (depends on: constants, types)
        │
        ├── kernel/quantum/chiral_gate.hpp           (core)
        ├── kernel/quantum/palindrome_precession.hpp (core)
        ├── kernel/ohm/coherence_duality.hpp         (core)
        │
        ├── kernel/kernel/arithmetic_periodicity.hpp (core)
        ├── kernel/kernel/memory.hpp                 (core)
        ├── kernel/kernel/interrupts.hpp             (core, memory)
        ├── kernel/kernel/ipc.hpp                    (core, memory)
        ├── kernel/kernel/process.hpp                (core, memory, ipc, interrupts, chiral_gate)
        ├── kernel/kernel/composition.hpp            (core, process)
        └── kernel/kernel/quantum_kernel.hpp         (all kernel/* headers)
                │
                ├── kernel/pipeline/kernel_state.hpp     (core)
                ├── kernel/pipeline/spectral_bridge.hpp  (core, kernel_state)
                └── kernel/pipeline/kernel_pipeline.hpp  (core, pipeline/*)
                        │
                        ├── kernel/oracle/master_eigen_oracle.hpp (core)
                        ├── kernel/quantum/ladder_search.hpp     (core, Eigen)
                        └── kernel/qudit/qudit_kernel.hpp        (core)
```

## Namespace Map

```
kernel::                              Top-level shared primitives
├── ETA, DELTA_S, PI, MU, ...        Constants and types
├── coherence(), lyapunov(), ...      Theorem functions
│
├── kernel::scheduling::              Process management
│   ├── QuantumKernel                 Orchestrator (boot, spawn, tick, compose)
│   ├── Process                       Quantum process with Z/8Z state
│   ├── ProcessComposition            Entanglement and interaction
│   ├── RotationalMemory              Z/8Z-addressed memory banks
│   ├── ArithmeticPeriodicity         Palindrome-derived periodicity
│   ├── ::ipc::QuantumIPC             Inter-process communication
│   └── ::interrupts::DecoherenceHandler  Interrupt detection & recovery
│
├── kernel::pipeline::                Coherence pipeline
│   ├── KernelState                   Invariant monitoring (µ, det R, R⁸=I)
│   ├── SpectralBridge                Mode switching (coherent/decoherent)
│   └── Pipeline                      End-to-end simulation
│
├── kernel::quantum::                 Quantum gates and search
│   ├── chiral_nonlinear()            Gate driven by µ = e^{i3π/4}
│   ├── PalindromePrecession          987654321/123456789 arithmetic periodicity
│   ├── LadderChiralSearch            Eigenspace-decomposed search (Eigen)
│   └── LadderSearchBenchmark         Benchmark utilities
│
├── kernel::oracle::                  Oracle framework
│   └── MasterEigenOracle             θ/√n search with 8 eigenspace channels
│
├── kernel::ohm::                     Circuit analogy
│   ├── CoherentChannel               Single coherent channel (C, G, R)
│   ├── MultiChannelSystem            N-channel parallel system
│   ├── PipelineSystem                Series pipeline of channels
│   ├── FourChannelModel              Silver-ratio structured model
│   ├── OUProcess                     Ornstein–Uhlenbeck noise model
│   ├── PhaseBattery                  Phase-driven energy storage
│   └── QuTritDegradation             Qutrit decoherence model
│
└── kernel::qudit::                   d-dimensional extension
    ├── QuditState                    d-level quantum state
    ├── QuditOps                      Generalized rotation, shift, clock
    ├── QuditEntangle                 Multi-qudit entanglement
    ├── QuditMemory                   d-bank memory addressing
    ├── QuditProcess                  d-cycle process management
    └── QuditKernel                   Full qudit orchestrator
```

## Data Flow: QState Lifecycle

```
                    Boot
                      │
              ┌───────▼───────┐
              │ verify_silver  │  δ_S · δ_conj = 1?
              │ _conservation()│
              └───────┬───────┘
                      │ pass
              ┌───────▼───────┐
              │  spawn(name,  │  Create Process with
              │    task_fn)   │  canonical QState (r=1)
              └───────┬───────┘
                      │
              ┌───────▼───────┐
              │   tick()      │  For each process:
              │               │  1. chiral_nonlinear() gate
              │               │  2. Advance cycle_pos mod 8
              │               │  3. Check coherence
              └───────┬───────┘
                      │
              ┌───────▼───────┐
              │  Coherence    │  classify_regime(r):
              │  Check        │   r=1 → FINITE_ORBIT
              │               │   r>1 → SPIRAL_OUT
              │               │   r<1 → SPIRAL_IN
              └───┬───┬───┬───┘
                  │   │   │
         r=1 ────┘   │   └──── r≠1 (decoherence)
         (ok)        │              │
                     │    ┌────────▼────────┐
                     │    │ DecoherenceHandler│
                     │    │   interrupt()    │
                     │    │                  │
                     │    │ MINOR: log       │
                     │    │ MAJOR: recover   │
                     │    │ CRITICAL: halt   │
                     │    └────────┬────────┘
                     │             │ recovered
                     └─────┬───────┘
                           │
                   ┌───────▼───────┐
                   │  Composition  │  Entangle processes,
                   │  & IPC        │  exchange messages
                   └───────────────┘
```

## Build Configuration

| CMake Option | Default | Description |
|-------------|---------|-------------|
| `KERNEL_BUILD_TESTS` | `ON` | Build unit and integration tests |
| `KERNEL_BUILD_BENCHMARKS` | `ON` | Build NIST IR 8356 benchmarks |
| `KERNEL_BUILD_EXAMPLES` | `ON` | Build demo executables |

### Requirements

- C++17 compiler (GCC 8+, Clang 7+, MSVC 19.14+)
- CMake 3.14+
- Eigen (optional, for `ladder_search.hpp`)
- OpenSSL (optional, for `benchmark_pow_nonce_search`)

### Build Commands

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cd build && ctest --output-on-failure
```
