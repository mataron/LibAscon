# CMake configuration to build LibAscon
# License: Creative Commons Zero (CC0) 1.0
#
# For example usage: see README.md

cmake_minimum_required(VERSION 3.6)
project(LibAscon
        VERSION 1.0.1
        LANGUAGES C
        DESCRIPTION
        "Lightweight Authenticated Encryption & Hashing, \
        also with Init-Update-Final paradigm.")

# Unless specified, by default create Release builds
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# -----------------------------------------------------------------------------
# Compiler flags
# -----------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (MSVC)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS_RELEASE} -D_MSVC")
endif ()

# Activate a million warnings to have the cleanest possible code
set(FLAGS_WARNINGS -Wall -Wextra -pedantic -Wconversion -Wdouble-promotion
        -Wswitch-default -Wswitch-enum -Wuninitialized -Wno-unused-variable
        -Wpacked -Wpadded -Wshadow -Wformat-security -Wlogical-not-parentheses
        -Waggregate-return -Wmissing-declarations -Wsign-conversion
        -Wjump-misses-init)
if (NOT MSVC)
# Debug build: compile with no optimisation, debug info and printing
set(CMAKE_C_FLAGS_DEBUG "${WARNING_FLAGS} -g -O0 -DDEBUG \
    -fprofile-instr-generate -fcoverage-mapping")
endif ()
# Append sanitiser flags on non-Windows systems
if (NOT WIN32 AND NOT CYGWIN AND NOT MSYS)
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} \
            -fsanitize=address,undefined -static-libsan \
            -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
endif ()

if (MSVC)
# Mini-sized release build: compile with optimisation for size
# convert warnings into errors and some other optimisations
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -DMINSIZEREL \
        ${WARNING_FLAGS}")

# Performance-oriented release build: compile with optimisation for speed
# convert warnings into errors and some other optimisations
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DRELEASE \
        ${WARNING_FLAGS}")
else ()
# Mini-sized release build: compile with optimisation for size
# convert warnings into errors and some other optimisations
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -DMINSIZEREL \
        ${WARNING_FLAGS} \
        -Os -Werror -fomit-frame-pointer -march=native -mtune=native -flto")

# Performance-oriented release build: compile with optimisation for speed
# convert warnings into errors and some other optimisations
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DRELEASE \
        ${WARNING_FLAGS} \
        -O3 -Werror -fomit-frame-pointer -march=native -mtune=native \
        -funroll-loops -flto")
endif ()

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
set(LIB_HEADER inc/ascon.h)
set(LIB_SRC_FILES_COMMON
        ${LIB_HEADER}
        src/ascon_internal.h
        src/ascon_permutations.c
        src/ascon_buffering.c)
set(LIB_SRC_FILES_128
        ${LIB_SRC_FILES_COMMON}
        src/ascon_aead_common.c
        src/ascon_aead128.c)
set(LIB_SRC_FILES_128a
        ${LIB_SRC_FILES_COMMON}
        src/ascon_aead_common.c
        src/ascon_aead128a.c)
set(LIB_SRC_FILES_80pq
        ${LIB_SRC_FILES_COMMON}
        src/ascon_aead_common.c
        src/ascon_aead128.c
        src/ascon_aead80pq.c
        )
set(LIB_SRC_FILES_HASH
        ${LIB_SRC_FILES_COMMON}
        src/ascon_hash.c)
set(LIB_SRC_FILES_FULL
        ${LIB_SRC_FILES_COMMON}
        ${LIB_SRC_FILES_128}
        ${LIB_SRC_FILES_128a}
        ${LIB_SRC_FILES_80pq}
        ${LIB_SRC_FILES_HASH}
        )
set(TEST_FRAMEWORK
        tst/atto/atto.c
        tst/main.c
        tst/vectors.c)
set(TEST_SRC_FILES
        tst/test_xof.c
        tst/test_hash.c
        tst/test_aead128_enc.c
        tst/test_aead128_dec.c
        tst/test_aead128_inplace.c
        tst/test_aead128_vartaglen.c
        tst/test_aead80pq_enc.c
        tst/test_aead80pq_dec.c
        tst/test_aead80pq_inplace.c
        tst/test_aead80pq_vartaglen.c
        tst/test_aead128a_enc.c
        tst/test_aead128a_dec.c
        tst/test_aead128a_inplace.c
        tst/test_aead128a_vartaglen.c
        tst/test_structs.c
        tst/test_readme_example.c
        )
set(BENCHMARK_SRC_FILES
        tst/benchmark.c)
include_directories(AFTER SYSTEM inc/ tst/atto/)


# -----------------------------------------------------------------------------
# Build targets
# ----------------------------------------------------------------------------
# Copy the library API header into the build directory.
# With this command, if the header changes, it's copied, otherwise not.
configure_file(${LIB_HEADER} ${CMAKE_BINARY_DIR} COPYONLY)

# Build the library first into an object, but keep `objlibascon` as
# a reference to it, so there is no need to recompile it when building
# a different target, that uses the same source files.
# Static libraries (.a), compiled with all features or only some features
add_library(asconfull STATIC ${LIB_SRC_FILES_FULL})
add_library(ascon128 STATIC ${LIB_SRC_FILES_128})
add_library(ascon128a STATIC ${LIB_SRC_FILES_128a})
add_library(ascon80pq STATIC ${LIB_SRC_FILES_80pq})
add_library(asconhash STATIC ${LIB_SRC_FILES_HASH})

# Shared library (.dylib / .dll)
# Does not reuse the static library object files, as they are
# recompiled in order to have position-independent code
add_library(ascon SHARED ${LIB_SRC_FILES_FULL})
set_property(TARGET ascon PROPERTY POSITION_INDEPENDENT_CODE ON)

# Test runner executable, reuse the static library instead of recompiling
# the source files
add_executable(testascon
        ${TEST_FRAMEWORK}
        ${TEST_SRC_FILES})
target_link_libraries(testascon PUBLIC asconfull)
target_include_directories(testascon PUBLIC inc/)

# Copy test vectors files to build directory after the make-build.
# They are used by the test runner
add_custom_command(
        TARGET testascon POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/tst/vectors
        ${CMAKE_CURRENT_BINARY_DIR}/vectors)
# Delete the test vectors file on make-clean
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        "${CMAKE_CURRENT_BINARY_DIR}/vectors")

#if (NOT MSVC) # TODO: find out why this has link errors...
# Benchmark runner executable, reuse the static library instead of recompiling
# the source files
add_executable(benchmark
        ${BENCHMARK_SRC_FILES})
target_link_libraries(benchmark PUBLIC asconfull)
target_include_directories(benchmark PUBLIC inc/)
#endif ()

# Doxygen documentation builder
find_package(Doxygen)
if (DOXYGEN_FOUND)
    # Cmake's wrapper of Doxygen, constructing a doxyfile from the
    # DOXYGEN_* variables, which are mapped to the Doxygen variables.
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_MAN YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_SORT_MEMBER_DOCS NO)
    set(DOXYGEN_IMAGE_PATH ${PROJECT_SOURCE_DIR}/images)
    set(DOXYGEN_ALIASES license="**License:**")
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    doxygen_add_docs(doxygen
            ALL # Build doxygen on make-all
            # List of input files for Doxygen
            ${PROJECT_SOURCE_DIR}/inc/ascon.h
            ${PROJECT_SOURCE_DIR}/LICENSE.md
            ${PROJECT_SOURCE_DIR}/AUTHORS.md
            ${PROJECT_SOURCE_DIR}/README.md
            ${PROJECT_SOURCE_DIR}/CHANGELOG.md)
else (DOXYGEN_FOUND)
    message(WARNING "Doxygen not found. Cannot generate documentation.")
endif (DOXYGEN_FOUND)
