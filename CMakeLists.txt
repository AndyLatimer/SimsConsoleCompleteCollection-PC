# The Sims 2 (original Xbox, USA), statically recompiled

cmake_minimum_required(VERSION 3.25)
project(sims2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# the SDK: sdk/ here, a checkout beside this repository, else downloaded
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/sdk/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp/cmake/XbeRecomp.cmake)
    set(SDK ${CMAKE_CURRENT_SOURCE_DIR}/../XboxXBERecomp)
else()
    include(FetchContent)
    FetchContent_Declare(xbe_sdk GIT_REPOSITORY https://github.com/TGP482/XboxXBERecomp.git GIT_TAG main)
    FetchContent_MakeAvailable(xbe_sdk)
    set(SDK ${xbe_sdk_SOURCE_DIR})
endif()
# host-tools preset: xbe_recomp and xbe_symbols alone, built for this machine. A cross build
# (windows-cross) runs xbe_recomp during the build and takes it from there (XBE_HOST_TOOLS_DIR).
option(XBE_HOST_TOOLS_ONLY "build only the SDK's host tools, not the game" OFF)
if(XBE_HOST_TOOLS_ONLY)
    include(${SDK}/cmake/XbeTools.cmake)
    return()
endif()
include(${SDK}/cmake/XbeRecomp.cmake)

file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/*/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/src/game(_pal)?/")

# The region is the disc in assets/: each retail xbe has its own config, recompiled code and
# address-bound hooks (docs/REGIONS.md). USA 16f9e41c... -> config/game.toml + src/game,
# Europe 68d7eabd... -> config/game_pal.toml + src/game_pal. SIMS2_REGION=USA|PAL overrides.
set(SIMS2_XBE_USA 16f9e41ce26c315a758f25d43ba1fa9d3809d1ad)
set(SIMS2_XBE_PAL 68d7eabdea89c9900e1a440c10bc3905e1afdf8b)
set(SIMS2_REGION "" CACHE STRING "USA or PAL; empty detects it from assets/default.xbe")
set(SIMS2_REGION_USED ${SIMS2_REGION})
if(NOT SIMS2_REGION_USED AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/default.xbe)
    file(SHA1 ${CMAKE_CURRENT_SOURCE_DIR}/assets/default.xbe sims2_sha1)
    if(sims2_sha1 STREQUAL SIMS2_XBE_PAL)
        set(SIMS2_REGION_USED PAL)
    elseif(sims2_sha1 STREQUAL SIMS2_XBE_USA)
        set(SIMS2_REGION_USED USA)
    else()
        message(WARNING "assets/default.xbe (SHA-1 ${sims2_sha1}) is neither the USA nor the Europe retail release")
    endif()
endif()
if(SIMS2_REGION_USED STREQUAL PAL)
    set(GAME_CONFIG game_pal.toml)
    set(GAME_DIR game_pal)
else()
    set(SIMS2_REGION_USED USA)
    set(GAME_CONFIG game.toml)
    set(GAME_DIR game)
endif()
message(STATUS "The Sims 2 region: ${SIMS2_REGION_USED} (config/${GAME_CONFIG}, src/${GAME_DIR})")

# partitions as xbe_symbols wrote them into the region's config
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS config/${GAME_CONFIG})
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config/${GAME_CONFIG})
    file(STRINGS config/${GAME_CONFIG} GAME_PARTITIONS REGEX "^partitions *=")
    string(REGEX MATCH "[0-9]+" GAME_PARTITIONS "${GAME_PARTITIONS}")
endif()
# no config yet: build.bat writes it with xbe_symbols, the change reconfigures
if(NOT GAME_PARTITIONS)
    set(GAME_PARTITIONS 8)
endif()

# target name = GAME_EXE in build.bat and release.bat, xbe = GAME_XBE
add_xbe_image(Sims2 default.xbe ${GAME_CONFIG} ${GAME_PARTITIONS} ${GAME_DIR})
target_compile_definitions(Sims2 PRIVATE SIMS2_REGION_${SIMS2_REGION_USED}=1)
