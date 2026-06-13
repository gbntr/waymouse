cmake_minimum_required(VERSION 3.22)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

enable_testing()

# Include paths for source code
include_directories(${CMAKE_SOURCE_DIR}/src)

# Helper to conditionally link stdc++fs (only needed on older GCC)
function(maybe_link_filesystem target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.1")
        target_link_libraries(${target} PRIVATE stdc++fs)
    endif()
endfunction()

# Test ThemeDetector (pure C++, no Qt dependency)
add_executable(test_theme_detector
    test_theme_detector.cpp
    ${CMAKE_SOURCE_DIR}/src/core/theme_detector.cpp
)
target_compile_features(test_theme_detector PRIVATE cxx_std_20)
maybe_link_filesystem(test_theme_detector)
add_test(NAME ThemeDetector COMMAND test_theme_detector)

# Test PointerManager (QTest)
add_executable(test_pointer_manager
    test_pointer_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/pointer_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/theme_detector.cpp
)
target_link_libraries(test_pointer_manager PRIVATE
    Qt6::Core
    Qt6::Test
)
target_compile_features(test_pointer_manager PRIVATE cxx_std_20)
add_test(NAME PointerManager COMMAND test_pointer_manager)

# Test PointerPanel (QTest — requires QApplication)
add_executable(test_pointer_panel
    test_pointer_panel.cpp
    ${CMAKE_SOURCE_DIR}/src/gui/pointer_panel.cpp
    ${CMAKE_SOURCE_DIR}/src/core/pointer_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/theme_detector.cpp
)
target_link_libraries(test_pointer_panel PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
)
target_compile_features(test_pointer_panel PRIVATE cxx_std_20)
add_test(NAME PointerPanel COMMAND test_pointer_panel)

# Test ConfigManager (pure C++) — only depends on toml11
add_executable(test_config_manager
    test_config_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/config_manager.cpp
)
target_link_libraries(test_config_manager PRIVATE
    toml11::toml11
)
target_compile_features(test_config_manager PRIVATE cxx_std_20)
maybe_link_filesystem(test_config_manager)
add_test(NAME ConfigManager COMMAND test_config_manager)

# Test ShakeDetector (pure C++)
add_executable(test_shake_detector
    test_shake_detector.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shake_detector.cpp
)
target_compile_features(test_shake_detector PRIVATE cxx_std_20)
add_test(NAME ShakeDetector COMMAND test_shake_detector)
