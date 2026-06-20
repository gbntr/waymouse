cmake_minimum_required(VERSION 3.22)
project(waymouse VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_GUI "Build Qt6 GUI" ON)

# Dependencies
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt6_standard_project_setup()

pkg_check_modules(WAYLAND REQUIRED wayland-client)
pkg_check_modules(UDEV REQUIRED libudev)
pkg_check_modules(LIBINPUT REQUIRED libinput)
pkg_check_modules(WLR_PROTOCOLS REQUIRED wayland-protocols)

# wayland-scanner for protocol generation
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

# Fetch toml11 (header-only)
include(FetchContent)
FetchContent_Declare(
    toml11
    GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
    GIT_TAG        v4.2.0
)
FetchContent_MakeAvailable(toml11)

# nlohmann/json (header-only)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# Sources

# Protocol generation (wlr-layer-shell-unstable-v1)
set(PROTOCOL_XML ${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml)
set(PROTOCOL_HEADER ${CMAKE_SOURCE_DIR}/src/wayland/wlr-layer-shell-unstable-v1-client-protocol.h)
set(PROTOCOL_CODE   ${CMAKE_SOURCE_DIR}/src/wayland/wlr-layer-shell-unstable-v1-client-protocol.c)

add_custom_command(
    OUTPUT ${PROTOCOL_HEADER}
    COMMAND ${WAYLAND_SCANNER} client-header ${PROTOCOL_XML} ${PROTOCOL_HEADER}
    DEPENDS ${PROTOCOL_XML}
    COMMENT "Generating layer-shell client header"
)

add_custom_command(
    OUTPUT ${PROTOCOL_CODE}
    COMMAND ${WAYLAND_SCANNER} private-code ${PROTOCOL_XML} ${PROTOCOL_CODE}
    DEPENDS ${PROTOCOL_XML}
    COMMENT "Generating layer-shell client code"
)

set(PROTOCOL_SOURCES ${PROTOCOL_HEADER} ${PROTOCOL_CODE})

set(CORE_SOURCES
    src/core/compositor_detector.cpp
    src/core/config_manager.cpp
    src/core/config_watcher.cpp
    src/core/device_manager.cpp
    src/core/mango_ipc_client.cpp
    src/core/runtime_lock.cpp
    src/core/runtime_status.cpp
    src/core/pointer_manager.cpp
    src/core/theme_detector.cpp
    src/core/shake_detector.cpp
    src/core/raw_input_monitor.cpp
    src/core/shake_manager.cpp
    src/core/shake_session_runtime.cpp
)

set(BACKEND_SOURCES
    src/backends/mango_backend.cpp
    src/backends/niri_backend.cpp
    src/backends/libinput_backend.cpp
    src/backends/env_cursor_backend.cpp
    src/backends/wlr_cursor_backend.cpp
    src/backends/niri_cursor_backend.cpp
)

set(GUI_SOURCES
    src/gui/main_window.cpp
    src/gui/device_panel.cpp
    src/gui/pointer_panel.cpp
    src/gui/shake_panel.cpp
    src/gui/shake_overlay.cpp
)

set(WAYLAND_SOURCES
    src/wayland/layer_shell_surface.cpp
    ${PROTOCOL_CODE}
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${BACKEND_SOURCES}
    ${GUI_SOURCES}
    ${WAYLAND_SOURCES}
)

# Executable
add_executable(waymouse ${ALL_SOURCES})

target_include_directories(waymouse PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/wayland
    ${WAYLAND_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
    ${LIBINPUT_INCLUDE_DIRS}
)

target_link_libraries(waymouse PRIVATE
    Qt6::Core
    Qt6::Widgets
    toml11::toml11
    nlohmann_json::nlohmann_json
    ${WAYLAND_LIBRARIES}
    ${UDEV_LIBRARIES}
    ${LIBINPUT_LIBRARIES}
    Threads::Threads
)

target_compile_options(waymouse PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install
install(TARGETS waymouse DESTINATION bin)
install(FILES docs/waymouse.1 DESTINATION share/man/man1 OPTIONAL)
