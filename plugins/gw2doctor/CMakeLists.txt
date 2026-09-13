cmake_minimum_required(VERSION 3.20)
project(gw2doctor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Separate addon from cba4gw2 on purpose (2026-09-13, Emi: "bauen wir ein
# eigenes Modul daraus") - startup/stability diagnostics are unrelated to
# color correction, and a standalone addon can still report useful things
# even if cba4gw2 itself fails to load.
add_library(gw2doctor SHARED
    src/ModuleMain.cpp
    src/Diagnostics.cpp
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
)

target_include_directories(gw2doctor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nexus-api
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui
)

target_link_libraries(gw2doctor PRIVATE
    d3d11.lib
    dxgi.lib
    psapi.lib
    shell32.lib
    ole32.lib
    powrprof.lib
)

set_target_properties(gw2doctor PROPERTIES
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Same auto-deploy convenience as cba4gw2's own CMakeLists.txt - skips with a
# warning instead of failing the build if GW2 still has the DLL loaded.
set(GW2_ADDONS_DIR "C:/GAMES/Guild Wars 2/addons")
if(EXISTS "${GW2_ADDONS_DIR}")
    add_custom_command(TARGET gw2doctor POST_BUILD
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/deploy_dll.ps1"
            -Source "$<TARGET_FILE:gw2doctor>"
            -Destination "${GW2_ADDONS_DIR}/gw2doctor.dll"
        COMMENT "Auto-deploying gw2doctor.dll to Guild Wars 2 addons directory for hot-reloading..."
    )
endif()
