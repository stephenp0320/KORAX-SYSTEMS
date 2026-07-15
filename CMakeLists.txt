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

# ---- Core library: libkorax ---------------------------------------------
add_library(korax STATIC
    src/raven/raven_decode.c
    src/rook/table_track.c
    src/capture/capture_ble.c
    src/pipeline/pipeline.c)
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

# ---- Tests ---------------------------------------------------------------
if(KORAX_BUILD_TESTS)
    enable_testing()
    add_executable(test_decode tests/unit/test_decode_full.c)
    target_link_libraries(test_decode PRIVATE korax)
    add_test(NAME decode_suite COMMAND test_decode)
endif()

# ---- Install -------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS korax
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/korax
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
