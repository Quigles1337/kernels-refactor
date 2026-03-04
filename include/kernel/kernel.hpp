/*
 * kernel.hpp — Umbrella Header for the Kernel Framework
 *
 * Including this single header brings in the complete Kernel API.
 * For finer-grained control, include individual module headers directly.
 *
 * Module hierarchy:
 *
 *   kernel/core/constants.hpp     — Mathematical constants (single source)
 *   kernel/core/types.hpp         — Cx, Vec2, QState, Regime, DecoherenceLevel
 *   kernel/core/theorems.hpp      — C(r), λ(r), R(r), rotate135(), etc.
 *
 *   kernel/kernel/memory.hpp      — RotationalMemory (Z/8Z addressing)
 *   kernel/kernel/interrupts.hpp  — DecoherenceHandler
 *   kernel/kernel/ipc.hpp         — QuantumIPC (inter-process communication)
 *   kernel/kernel/process.hpp     — Process struct
 *   kernel/kernel/composition.hpp — ProcessComposition
 *   kernel/kernel/quantum_kernel.hpp — QuantumKernel orchestrator
 *
 *   kernel/pipeline/kernel_state.hpp    — KernelState (invariant enforcement)
 *   kernel/pipeline/spectral_bridge.hpp — SpectralBridge
 *   kernel/pipeline/kernel_pipeline.hpp — Pipeline (end-to-end)
 *
 *   kernel/quantum/chiral_gate.hpp           — ChiralNonlinearGate
 *   kernel/quantum/palindrome_precession.hpp — PalindromePrecession
 *   kernel/quantum/ladder_search.hpp         — LadderChiralSearch
 *
 *   kernel/oracle/master_eigen_oracle.hpp — MasterEigenOracle
 *   kernel/ohm/coherence_duality.hpp      — Ohm–Coherence Duality
 *   kernel/qudit/qudit_kernel.hpp         — Qudit extension
 *
 * Copyright (c) 2024–2026 beanapologist / COINjecture Network / DARQ Labs
 */

#pragma once

// ── Core: always available ───────────────────────────────────────────────────
#include <kernel/core/constants.hpp>
#include <kernel/core/types.hpp>
#include <kernel/core/theorems.hpp>

// ── Quantum gates ────────────────────────────────────────────────────────────
#include <kernel/quantum/chiral_gate.hpp>
#include <kernel/quantum/palindrome_precession.hpp>

// ── Ohm–Coherence Duality ───────────────────────────────────────────────────
#include <kernel/ohm/coherence_duality.hpp>

// ── Kernel scheduling ───────────────────────────────────────────────────────
#include <kernel/kernel/arithmetic_periodicity.hpp>
#include <kernel/kernel/memory.hpp>
#include <kernel/kernel/interrupts.hpp>
#include <kernel/kernel/ipc.hpp>
#include <kernel/kernel/process.hpp>
#include <kernel/kernel/composition.hpp>
#include <kernel/kernel/quantum_kernel.hpp>

// ── Pipeline ────────────────────────────────────────────────────────────────
#include <kernel/pipeline/kernel_state.hpp>
#include <kernel/pipeline/spectral_bridge.hpp>
#include <kernel/pipeline/kernel_pipeline.hpp>

// ── Oracle ──────────────────────────────────────────────────────────────────
#include <kernel/oracle/master_eigen_oracle.hpp>

// ── Qudit extension ─────────────────────────────────────────────────────────
#include <kernel/qudit/qudit_kernel.hpp>
