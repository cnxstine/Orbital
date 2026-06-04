# =============================================================================
# cmake/CompilerOptions.cmake
#
# Defines the `orbital_compiler_options` INTERFACE target.
# All Orbital library targets link this to inherit consistent flags.
#
# This approach keeps flags centralized and avoids polluting third-party
# targets (which we do NOT link orbital_compiler_options into).
# =============================================================================

add_library(orbital_compiler_options INTERFACE)

# ── GCC / Clang / AppleClang ──────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(orbital_compiler_options INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Wno-unused-parameter        # Suppressed: virtual stubs are common
        -Wno-missing-field-initializers
        -Werror=return-type
        -Werror=uninitialized
    )

    target_compile_options(orbital_compiler_options INTERFACE
        # Debug: full debug info, no inlining, frame pointer for profilers
        $<$<CONFIG:Debug>:
            -O0
            -g3
            -fno-omit-frame-pointer
            -fno-inline
        >
        # RelWithDebInfo: optimised but still debuggable
        $<$<CONFIG:RelWithDebInfo>:
            -O2
            -g
            -fno-omit-frame-pointer
        >
        # Release: maximum performance
        $<$<CONFIG:Release>:
            -O3
            -march=native
            -flto
            -DNDEBUG
        >
    )

    # Address + Undefined Behaviour sanitizers (Debug only, opt-out available)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        option(ORBITAL_SANITIZERS "Enable ASan + UBSan in Debug builds" OFF)
        if(ORBITAL_SANITIZERS)
            message(STATUS "[Orbital] Sanitizers: ASan + UBSan ENABLED")
            target_compile_options(orbital_compiler_options INTERFACE
                -fsanitize=address,undefined
                -fno-sanitize-recover=all
            )
            target_link_options(orbital_compiler_options INTERFACE
                -fsanitize=address,undefined
            )
        endif()
    endif()

# ── MSVC ──────────────────────────────────────────────────────────────────────
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(orbital_compiler_options INTERFACE
        /W4
        /permissive-                 # Standards conformance mode
        /Zc:__cplusplus              # Report correct __cplusplus value
        /Zc:preprocessor             # New conformant preprocessor
        /utf-8                       # Source and execution charset UTF-8
        /wd4100                      # Suppress: unreferenced formal parameter
        /wd4505                      # Suppress: unreferenced local function
        $<$<CONFIG:Debug>:
            /Od
            /Zi
            /RTC1                    # Runtime checks
        >
        $<$<CONFIG:RelWithDebInfo>:
            /O2
            /Zi
        >
        $<$<CONFIG:Release>:
            /O2
            /GL                      # Whole-programme optimisation
            /DNDEBUG
        >
    )
    target_link_options(orbital_compiler_options INTERFACE
        $<$<CONFIG:Release>:/LTCG>
    )
endif()
