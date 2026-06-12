cmake_minimum_required(VERSION 3.22)
project(waymouse VERSION 0.1.0 LANGUAGES CXX)

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
set(CORE_SOURCES
    src/core/compositor_detector.cpp
    src/core/config_manager.cpp
    src/core/device_manager.cpp
    src/core/pointer_manager.cpp
    src/core/theme_detector.cpp
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
)

set(ALL_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${BACKEND_SOURCES}
    ${GUI_SOURCES}
)

# Executable
add_executable(waymouse ${ALL_SOURCES})

target_include_directories(waymouse PRIVATE
    ${CMAKE_SOURCE_DIR}/src
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
