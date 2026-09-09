# SPDX-License-Identifier: GPL-3.0-or-later
set(TC002_SDK_ROOT "${CMAKE_SOURCE_DIR}/.cache/tooling/sdk" CACHE PATH "Extracted pinned FlyThings packages")
set(_tc002_packages easyui log zkhardware zknet base-utility audio-utility base-json ext4 ffmpeg z curl-mbedtls mbedtls cares mi_ao mi_aio mi_sys mi_common cam_os_wrapper apc aec rapidjson)
add_library(flythings_sdk INTERFACE)
target_compile_definitions(flythings_sdk INTERFACE __PLATFORM_Z21__=1)
foreach(_package IN LISTS _tc002_packages)
 if(EXISTS "${TC002_SDK_ROOT}/${_package}/include")
  target_include_directories(flythings_sdk SYSTEM INTERFACE "${TC002_SDK_ROOT}/${_package}/include")
 endif()
endforeach()
foreach(_library easyui log zkhardware zknet mi_ao mi_sys mi_common cam_os_wrapper)
 set(_path "${TC002_SDK_ROOT}/${_library}/lib/lib${_library}.so")
 if(NOT EXISTS "${_path}")
  message(FATAL_ERROR "Missing TC002 SDK library: ${_path}. Run scripts/build-tc002.py --prepare first.")
 endif()
 target_link_libraries(flythings_sdk INTERFACE "${_path}")
endforeach()
target_link_libraries(flythings_sdk INTERFACE
 "-Wl,--start-group"
 "${TC002_SDK_ROOT}/audio-utility/lib/libaudio-utility.a"
 "${TC002_SDK_ROOT}/base-utility/lib/libbase-utility.a"
 "${TC002_SDK_ROOT}/base-json/lib/libbase-json.a"
 "${TC002_SDK_ROOT}/ext4/lib/libext4.a"
 "${TC002_SDK_ROOT}/ffmpeg/lib/libswresample.a"
 "${TC002_SDK_ROOT}/ffmpeg/lib/libavutil.a"
 "-Wl,--end-group" dl rt m pthread)

if(NOT TARGET CURL::libcurl)
 add_library(CURL::libcurl STATIC IMPORTED GLOBAL)
 set_target_properties(CURL::libcurl PROPERTIES
  IMPORTED_LOCATION "${TC002_SDK_ROOT}/curl-mbedtls/lib/libcurl.a"
  INTERFACE_INCLUDE_DIRECTORIES "${TC002_SDK_ROOT}/curl-mbedtls/include"
  INTERFACE_LINK_LIBRARIES "${TC002_SDK_ROOT}/mbedtls/lib/libmbedtls.a;${TC002_SDK_ROOT}/mbedtls/lib/libmbedx509.a;${TC002_SDK_ROOT}/mbedtls/lib/libmbedcrypto.a;${TC002_SDK_ROOT}/mbedtls/lib/libeverest.a;${TC002_SDK_ROOT}/mbedtls/lib/libp256m.a;${TC002_SDK_ROOT}/cares/lib/libcares.a;${TC002_SDK_ROOT}/z/lib/libz.a;dl;pthread")
endif()
