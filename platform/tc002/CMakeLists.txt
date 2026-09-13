# SPDX-License-Identifier: GPL-3.0-or-later
add_library(owlanzi_tc002_platform STATIC src/tc002_platform.cpp)
target_include_directories(owlanzi_tc002_platform PUBLIC include)
target_link_libraries(owlanzi_tc002_platform PUBLIC flythings_sdk Threads::Threads)
add_executable(ota-switch src/ota_switch.cpp)
add_executable(owlanzi-boot-control src/boot_control.cpp)
add_executable(owlanzi-install-guard src/install_guard.cpp)
target_include_directories(owlanzi-install-guard PRIVATE include "${CMAKE_SOURCE_DIR}/vendor")
target_link_options(owlanzi-install-guard PRIVATE -Wl,-z,defs -Wl,-z,relro -Wl,-z,now)
set_target_properties(owlanzi-install-guard PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "")
target_include_directories(owlanzi-boot-control PRIVATE "${CMAKE_SOURCE_DIR}/vendor")
target_link_options(owlanzi-boot-control PRIVATE -Wl,-z,relro -Wl,-z,now)
set_target_properties(owlanzi-boot-control PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "")
add_library(owlanzi-boot SHARED src/boot_entry.cpp)
target_link_options(owlanzi-boot PRIVATE -Wl,-z,defs -Wl,-z,relro -Wl,-z,now)
target_link_options(ota-switch PRIVATE -Wl,-z,relro -Wl,-z,now)
add_library(zkgui SHARED src/flythings_entry.cpp src/main_activity.cpp src/app_bridge.cpp src/wifi_onboarding.cpp src/network_time.cpp src/update_platform.cpp)
target_link_libraries(zkgui PRIVATE owlanzi_tc002_platform owlanzi_runtime)
set(TC002_DEVICE_ABI_DIR "" CACHE PATH "Optional libraries read from the local test clock (link only)")
if(TC002_DEVICE_ABI_DIR)
 if(NOT EXISTS "${TC002_DEVICE_ABI_DIR}/libstdc++.so.6")
  message(FATAL_ERROR "Device ABI directory must contain the clock's libstdc++.so.6")
 endif()
 # Use the stock library's actual symbol versions instead of the compiler's
 # newer exception_ptr version map. Never copy a runtime library to the clock.
 target_link_libraries(zkgui PRIVATE "${TC002_DEVICE_ABI_DIR}/libstdc++.so.6")
 target_link_libraries(owlanzi-boot-control PRIVATE "${TC002_DEVICE_ABI_DIR}/libstdc++.so.6" "${TC002_DEVICE_ABI_DIR}/libm.so.6")
 target_link_libraries(owlanzi-install-guard PRIVATE "${TC002_DEVICE_ABI_DIR}/libstdc++.so.6" "${TC002_DEVICE_ABI_DIR}/libm.so.6")
endif()
target_link_options(zkgui PRIVATE -Wl,-z,defs -Wl,-z,now -Wl,-z,relro "-Wl,-Map,${CMAKE_BINARY_DIR}/tc002-link.map")
set_target_properties(zkgui PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE INSTALL_RPATH "$ORIGIN")
add_custom_command(TARGET zkgui POST_BUILD
 COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/device/lib" "${CMAKE_BINARY_DIR}/device/ui"
 COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:zkgui>" "${CMAKE_BINARY_DIR}/device/lib/libzkgui.so"
 COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/resources/main.ftu" "${CMAKE_BINARY_DIR}/device/ui/main.ftu"
 COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/.cache/tooling/cacert.pem" "${CMAKE_BINARY_DIR}/device/ui/cacert.pem"
 VERBATIM)
