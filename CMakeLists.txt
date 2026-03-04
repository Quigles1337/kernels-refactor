cmake_minimum_required(VERSION 3.14)

project(Kernel
    VERSION 2.0.0
    DESCRIPTION "Quantum Kernel — Pipeline of Coherence"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Installation paths ───────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ── Header-only library ──────────────────────────────────────────────────────
add_library(Kernel INTERFACE)
add_library(Kernel::Kernel ALIAS Kernel)

target_include_directories(Kernel INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(Kernel INTERFACE cxx_std_17)

# ── Benchmarks ───────────────────────────────────────────────────────────────
option(KERNEL_BUILD_BENCHMARKS "Build benchmark executables" ON)

if(KERNEL_BUILD_BENCHMARKS)
    add_executable(benchmark_nist_ir8356 benchmarks/benchmark_nist_ir8356.cpp)
    target_link_libraries(benchmark_nist_ir8356 PRIVATE Kernel::Kernel)
    if(UNIX)
        target_link_libraries(benchmark_nist_ir8356 PRIVATE m)
    endif()

    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        add_executable(benchmark_pow_nonce_search benchmarks/benchmark_pow_nonce_search.cpp)
        target_link_libraries(benchmark_pow_nonce_search PRIVATE Kernel::Kernel OpenSSL::Crypto)
        if(UNIX)
            target_link_libraries(benchmark_pow_nonce_search PRIVATE m)
        endif()
    else()
        message(STATUS "OpenSSL not found — skipping benchmark_pow_nonce_search")
    endif()

    add_executable(benchmark_scaling_falsification benchmarks/benchmark_scaling_falsification.cpp)
    target_link_libraries(benchmark_scaling_falsification PRIVATE Kernel::Kernel)
    if(UNIX)
        target_link_libraries(benchmark_scaling_falsification PRIVATE m)
    endif()
endif()

# ── Tests ────────────────────────────────────────────────────────────────────
option(KERNEL_BUILD_TESTS "Build test executables and register with CTest" ON)

if(KERNEL_BUILD_TESTS)
    enable_testing()

    # Core header smoke test (Phase 1 foundation)
    add_executable(test_core_headers tests/unit/test_core_headers.cpp)
    target_link_libraries(test_core_headers PRIVATE Kernel::Kernel)
    if(UNIX)
        target_link_libraries(test_core_headers PRIVATE m)
    endif()
    add_test(NAME test_core_headers COMMAND test_core_headers)

    # Unit tests
    set(_kernel_unit_tests
        test_chiral_nonlinear_gate
        test_coherent_search
        test_interrupt_nist
        test_ipc
        test_palindrome_precession
    )

    foreach(_test IN LISTS _kernel_unit_tests)
        add_executable(${_test} tests/unit/${_test}.cpp)
        target_link_libraries(${_test} PRIVATE Kernel::Kernel)
        if(UNIX)
            target_link_libraries(${_test} PRIVATE m)
        endif()
        add_test(NAME ${_test} COMMAND ${_test})
    endforeach()

    # Integration tests
    set(_kernel_integration_tests
        test_battery_analogy
        test_bft_extended
        test_bft_stress
        test_kernel_pipeline
        test_master_eigen_oracle
        test_noise_scaling_phase_transition
        test_ohm_coherence
        test_pipeline_theorems
        test_qudit_kernel
    )

    foreach(_test IN LISTS _kernel_integration_tests)
        add_executable(${_test} tests/integration/${_test}.cpp)
        target_link_libraries(${_test} PRIVATE Kernel::Kernel)
        if(UNIX)
            target_link_libraries(${_test} PRIVATE m)
        endif()
        add_test(NAME ${_test} COMMAND ${_test})
    endforeach()
endif()

# ── Examples ─────────────────────────────────────────────────────────────────
option(KERNEL_BUILD_EXAMPLES "Build example executables" ON)

if(KERNEL_BUILD_EXAMPLES)
    add_executable(demo_quantum_kernel examples/demo_quantum_kernel.cpp)
    target_link_libraries(demo_quantum_kernel PRIVATE Kernel::Kernel)
    if(UNIX)
        target_link_libraries(demo_quantum_kernel PRIVATE m)
    endif()

    add_executable(demo_qudit_kernel examples/demo_qudit_kernel.cpp)
    target_link_libraries(demo_qudit_kernel PRIVATE Kernel::Kernel)
    if(UNIX)
        target_link_libraries(demo_qudit_kernel PRIVATE m)
    endif()
endif()

# ── Installation ─────────────────────────────────────────────────────────────
install(DIRECTORY include/kernel
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS Kernel
    EXPORT KernelTargets
)

install(EXPORT KernelTargets
    FILE KernelTargets.cmake
    NAMESPACE Kernel::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Kernel
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/KernelConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KernelConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Kernel
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KernelConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KernelConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KernelConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Kernel
)
