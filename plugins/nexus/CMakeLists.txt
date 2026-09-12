cmake_minimum_required(VERSION 3.20)
project(cba LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ON by default: this is the local dev build (see ModuleMain.cpp GetAddonDef).
# Turn OFF only when actually cutting a release build meant to ship with
# Nexus's real GitHub-provider auto-update behavior.
option(CBA_LOCAL_DEV "Disable Nexus auto-update while iterating locally" ON)

# Build-time stamp for AddonVersion.Build (2026-09-09) - Emi's idea, because
# "is this DLL actually the one I just built" turned into a whole afternoon of
# byte-comparing files after something (Nexus's own Library auto-update, it
# looks like) kept silently reverting addons/cba.dll to an older build. This
# regenerates every single build (it's a target with no tracked OUTPUT, so
# CMake always re-runs it, unlike configure_file which only updates when
# CMake reconfigures) and stamps a simple +1-per-local-build counter (was
# hour/minute*100+second until 2026-09-11, see gen_buildstamp.ps1) - so the
# version number Nexus displays in its addon list changes on every real
# rebuild and can be eyeballed directly instead of comparing file sizes.
add_custom_target(cba_gen_buildstamp
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_buildstamp.ps1"
        -OutFile "${CMAKE_CURRENT_SOURCE_DIR}/src/BuildStamp.h"
    BYPRODUCTS "${CMAKE_CURRENT_SOURCE_DIR}/src/BuildStamp.h"
    COMMENT "Stamping build version (Build=+1 local build counter)..."
)

add_library(cba SHARED
    src/ModuleMain.cpp
    src/core/Shared.cpp
    src/core/ColorMatrix.cpp
    src/core/Settings.cpp
    src/core/ParameterRegistry.cpp
    src/core/SelfTest.cpp
    src/core/FeatureModule.cpp
    src/core/FilterLayers.cpp
    src/core/NexusEcosystem.cpp
    src/platform/Magnification.cpp
    src/platform/ShaderColorPipeline.cpp
    src/platform/FilterSensor.cpp
    src/platform/WindowMode.cpp
    src/platform/HybridScanner.cpp
    src/ui/UIState.cpp
    src/ui/Theme.cpp
    src/ui/MainWindow.cpp
    src/ui/MiniHUD.cpp
    src/ui/SensorGraphHUD.cpp
    src/ui/FilterLab.cpp
    src/ui/VisionLab.cpp
    src/ui/CreditsDialog.cpp
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
)

add_dependencies(cba cba_gen_buildstamp)

if(CBA_LOCAL_DEV)
    target_compile_definitions(cba PRIVATE CBA_LOCAL_DEV)
endif()

# Optional release-version override (2026-09-10) - release.yml passes the
# actual git tag here (e.g. -DCBA_RELEASE_TAG=v1.10.1). Without this,
# AddonDef.Version stayed hardcoded at a stale placeholder (1.0.2.0) forever
# in every shipped build, no matter what tag was actually published - Nexus
# compares its own parsed tag version (AddonVersion.cpp, 3-part semver:
# Major.Minor.Build) against this self-reported value to decide "is an
# update available," so a frozen version number meant Nexus always thought
# an update was available, even immediately after installing the latest one.
# Empty/unset (the default, and every local dev build) leaves the old
# hardcoded-version behavior in ModuleMain.cpp completely untouched.
set(CBA_RELEASE_TAG "" CACHE STRING "Git tag this build corresponds to (e.g. v1.10.1) - sets AddonDef.Version to match")
if(CBA_RELEASE_TAG)
    string(REGEX REPLACE "^v" "" CBA_VERSION_CLEAN "${CBA_RELEASE_TAG}")
    string(REPLACE "." ";" CBA_VERSION_PARTS "${CBA_VERSION_CLEAN}")
    list(LENGTH CBA_VERSION_PARTS CBA_VERSION_PARTS_LEN)
    if(CBA_VERSION_PARTS_LEN GREATER_EQUAL 1)
        list(GET CBA_VERSION_PARTS 0 CBA_VER_MAJOR)
        set(CBA_VER_MINOR 0)
        set(CBA_VER_BUILD 0)
        if(CBA_VERSION_PARTS_LEN GREATER_EQUAL 2)
            list(GET CBA_VERSION_PARTS 1 CBA_VER_MINOR)
        endif()
        if(CBA_VERSION_PARTS_LEN GREATER_EQUAL 3)
            list(GET CBA_VERSION_PARTS 2 CBA_VER_BUILD)
        endif()
        target_compile_definitions(cba PRIVATE
            CBA_VER_MAJOR=${CBA_VER_MAJOR}
            CBA_VER_MINOR=${CBA_VER_MINOR}
            CBA_VER_BUILD=${CBA_VER_BUILD}
        )
    endif()
endif()

target_include_directories(cba PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nexus-api
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui
)

target_link_libraries(cba PRIVATE
    Magnification.lib
    d3d11.lib
    dxgi.lib
)

set_target_properties(cba PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Unit tests for the pure-math core (core/ColorMatrix.cpp) - the one part of
# this addon with zero ImGui/Nexus/Win32-runtime dependency, so the one part
# that can run standalone without a live GW2 process. See tests/ for what's
# covered and why (invariants, not golden numbers). Runs automatically after
# every build (POST_BUILD) so a math regression shows up immediately instead
# of only "by inspection" during a later audit.
add_executable(cba_tests
    tests/test_color_matrix.cpp
    src/core/ColorMatrix.cpp
)
target_include_directories(cba_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)
set_target_properties(cba_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
add_custom_command(TARGET cba_tests POST_BUILD
    COMMAND cba_tests
    COMMENT "Running cba_tests..."
)

# Automatic hot-load deployment to local Guild Wars 2 addons directory.
# Uses tools/deploy_dll.ps1 (rather than a plain copy_if_different) only so
# a locked destination (GW2/Nexus still has cba.dll loaded) is skipped with
# a warning instead of failing the whole build.
set(GW2_ADDONS_DIR "C:/GAMES/Guild Wars 2/addons")
if(EXISTS "${GW2_ADDONS_DIR}")
    add_custom_command(TARGET cba POST_BUILD
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy_dll.ps1"
            -Source "$<TARGET_FILE:cba>"
            -Destination "${GW2_ADDONS_DIR}/cba.dll"
        COMMENT "Auto-deploying cba.dll to Guild Wars 2 addons directory for hot-reloading..."
    )
endif()
