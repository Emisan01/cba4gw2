cmake_minimum_required(VERSION 3.20)
project(cba LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cba SHARED
    src/ModuleMain.cpp
    src/core/Shared.cpp
    src/core/ColorMatrix.cpp
    src/core/Settings.cpp
    src/platform/Magnification.cpp
    src/platform/WindowMode.cpp
    src/platform/HybridScanner.cpp
    src/ui/UIState.cpp
    src/ui/MainWindow.cpp
    src/ui/SensorGraphHUD.cpp
    src/ui/FilterLab.cpp
    src/ui/VisionLab.cpp
    src/ui/SafeStartGate.cpp
    src/ui/CreditsDialog.cpp
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
)

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

# Automatic hot-load deployment to local Guild Wars 2 addons directory
set(GW2_ADDONS_DIR "C:/GAMES/Guild Wars 2/addons")
if(EXISTS "${GW2_ADDONS_DIR}")
    add_custom_command(TARGET cba POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:cba>
            "${GW2_ADDONS_DIR}/cba.dll"
        COMMENT "Auto-deploying cba.dll to Guild Wars 2 addons directory for hot-reloading..."
    )
endif()
