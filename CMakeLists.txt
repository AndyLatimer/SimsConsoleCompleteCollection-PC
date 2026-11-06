# The Sims (original Xbox, USA), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(Sims1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the SDK revision recorded by the submodule, never a different sibling or moving main.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
else()
    message(FATAL_ERROR "SDK missing. Run: git submodule update --init --recursive")
endif()
# host-tools preset: xbe_recomp and xbe_symbols alone, built for this machine. A cross build
# (windows-cross) runs xbe_recomp during the build and takes it from there (XBE_HOST_TOOLS_DIR).
option(XBE_HOST_TOOLS_ONLY "build only the SDK's host tools, not the game" OFF)
if(XBE_HOST_TOOLS_ONLY)
    include(${SDK}/cmake/XbeTools.cmake)
    return()
endif()

# This config and the game hooks contain USA addresses. Hashing an arbitrary PAL XBE into
# kImageSha1 would otherwise make the runtime accept it with the wrong recompiled hooks.
set(SIMS1_XBE "${CMAKE_CURRENT_SOURCE_DIR}/assets/default.xbe")
if(NOT EXISTS "${SIMS1_XBE}")
    message(FATAL_ERROR "Game data missing. Run: python tools/game_data.py import <Xbox ISO or extracted directory>. Tools alone: cmake --preset tools-release")
endif()
file(SHA1 "${SIMS1_XBE}" SIMS1_XBE_SHA1)
set(SIMS1_USA_HASHES
    "40ab6f97e8ee3bf4102a5a7820dcb8047fc30097"
    # User's USA disc: all 128 named SDK functions and game hook addresses audited.
    # docs/USA_DISC_VARIANT.md records evidence and the remaining runtime validation.
    "9e0526fc2ec6ad4896363486a404949ee3bea53f")
if(NOT SIMS1_XBE_SHA1 IN_LIST SIMS1_USA_HASHES)
    message(FATAL_ERROR "Unsupported default.xbe SHA-1: ${SIMS1_XBE_SHA1}. This config requires USA 40ab6f97e8ee3bf4102a5a7820dcb8047fc30097. Inspect PAL separately; do not apply USA hooks to it.")
endif()
include(${SDK}/cmake/XbeRecomp.cmake)

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/game/")

# partitions as xbe_symbols wrote them into config/game.toml
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/game.toml)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/game.toml)
    file(STRINGS config/game.toml GAME_PARTITIONS REGEX "^partitions *=")
    string(REGEX MATCH "[0-9]+" GAME_PARTITIONS "${GAME_PARTITIONS}")
endif()
# no config yet: build.bat writes it with xbe_symbols, the change reconfigures
if(NOT GAME_PARTITIONS)
    set(GAME_PARTITIONS 8)
endif()

# target name = GAME_EXE in build.bat and release.bat, xbe = GAME_XBE; add_xbe_image reads
# assets/default.xbe and config/game.toml, and adds src/game/*.cpp to this image alone
add_xbe_image(Sims1 default.xbe game.toml ${GAME_PARTITIONS} game)
