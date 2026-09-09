# SPDX-License-Identifier: GPL-3.0-or-later
add_library(owlanzi_tc002_platform STATIC src/tc002_platform.cpp src/sdk_wifi.cpp)
target_include_directories(owlanzi_tc002_platform PUBLIC include)
target_link_libraries(owlanzi_tc002_platform PUBLIC flythings_sdk Threads::Threads)
add_library(zkgui SHARED src/flythings_entry.cpp src/main_activity.cpp src/app_bridge.cpp)
target_link_libraries(zkgui PRIVATE owlanzi_tc002_platform owlanzi_runtime)
target_link_options(zkgui PRIVATE -Wl,-z,defs -Wl,-z,now -Wl,-z,relro)
set_target_properties(zkgui PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "$ORIGIN")
add_custom_command(TARGET zkgui POST_BUILD
 COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/device/lib" "${CMAKE_BINARY_DIR}/device/ui"
 COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:zkgui>" "${CMAKE_BINARY_DIR}/device/lib/libzkgui.so"
 COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/resources/main.ftu" "${CMAKE_BINARY_DIR}/device/ui/main.ftu"
 COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/.cache/tooling/cacert.pem" "${CMAKE_BINARY_DIR}/device/ui/cacert.pem"
 VERBATIM)
