cmake_minimum_required(VERSION 3.16)
project(korax
        VERSION 0.1.0
        DESCRIPTION "Remote ID decode, track and validation engine"
        LANGUAGES C)
# ---- Options -------------------------------------------------------------
option(KORAX_BUILD_TESTS    "Build the unit tests"   ON)
option(KORAX_BUILD_TOOLS    "Build the replay tool"  ON)
option(KORAX_BUILD_EXAMPLES "Build the examples"     ON)
option(KORAX_WERROR         "Treat warnings as errors" OFF)
option(KORAX_BUILD_FUZZERS  "Build the fuzzing tests" OFF)
# ---- Core library: libkorax ---------------------------------------------
add_library(korax STATIC
    src/raven/raven_decode.c
    src/rook/table_track.c
    src/capture/capture_ble.c
    src/pipeline/pipeline.c
    src/capture/capture_file.c
    src/magpie/magpie_validator.c
    src/output/output_json.c
    src/output/server.c
    src/scanner/scan.c
)
add_library(korax::korax ALIAS korax)
target_include_directories(korax
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_compile_features(korax PUBLIC c_std_11)
target_compile_options(korax PRIVATE -Wall -Wextra)
if(KORAX_WERROR)
    target_compile_options(korax PRIVATE -Werror)
endif()
# Rook's jump detection uses libm (sin/asin/sqrt).
include(CheckLibraryExists)
check_library_exists(m sin "" HAVE_LIBM)
if(HAVE_LIBM)
    target_link_libraries(korax PUBLIC m)
endif()
# scan.c's BlueZ code is compiled on any Linux build (guarded by #ifdef __linux__).
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(BLUETOOTH_LIBRARY bluetooth REQUIRED)
    target_link_libraries(korax PUBLIC ${BLUETOOTH_LIBRARY})
endif()
# ---- Tools ---------------------------------------------------------------
if(KORAX_BUILD_TOOLS)
    add_executable(korax_replay tools/replay/korax_replay.c)
    target_link_libraries(korax_replay PRIVATE korax)
endif()
# ---- Examples ------------------------------------------------------------
if(KORAX_BUILD_EXAMPLES)
    add_executable(korax_demo examples/demo_decode.c)
    target_link_libraries(korax_demo PRIVATE korax)
endif()
# ---- Daemon --------------------------------------------------------------
option(KORAX_BUILD_DAEMON "Build the koraxd daemon" ON)
if(KORAX_BUILD_DAEMON)
    add_executable(koraxd daemon/koraxd.c daemon/config.c daemon/log.c)
    target_link_libraries(koraxd PRIVATE korax)
endif()
# ---- Tests ---------------------------------------------------------------
if(KORAX_BUILD_TESTS)
    enable_testing()
    add_executable(test_decode tests/unit/test_decode_full.c daemon/config.c daemon/log.c)
    target_link_libraries(test_decode PRIVATE korax)
    add_test(NAME decode_suite COMMAND test_decode)
endif()
# ---- Fuzzers ---------------------------------------------------------------
# libFuzzer is a Clang feature; on any other compiler this quietly skips
# the fuzz targets rather than breaking the build.
if(KORAX_BUILD_FUZZERS)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(WARNING "KORAX_BUILD_FUZZERS requires Clang (libFuzzer); skipping fuzz targets on ${CMAKE_C_COMPILER_ID}")
    else()
        add_executable(fuzz_decode_message tests/fuzz/fuzz_decode_message.c)
        target_link_libraries(fuzz_decode_message PRIVATE korax)
        target_compile_options(fuzz_decode_message PRIVATE -fsanitize=fuzzer,address,undefined)
        target_link_options(fuzz_decode_message PRIVATE -fsanitize=fuzzer,address,undefined)
    endif()
endif()
# ---- Install -------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS korax
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/korax
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
