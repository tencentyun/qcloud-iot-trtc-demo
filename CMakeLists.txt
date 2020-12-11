# 项目信息
project (QCloud_IoT_SDK C CXX)

# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.5)

# 设置CMAKE使用编译工具及编译选项
set(CMAKE_SYSTEM_NAME  Linux)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH ${TOOL_CHAIN_PATH})

set(CMAKE_C_COMPILER  gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wfatal-errors -Os -pthread")

set(CMAKE_CXX_COMPILER  g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX_SDK_VER_MAIN=1 -DLINUX_SDK_VER_SUB=0 -DLINUX_SDK_VER_SVN=10000 -O0 -s")

# 设置编译相关变量
link_directories(${PROJECT_SOURCE_DIR}/trtc_sdk/trtc_6.5.70/lib)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/trtc_sdk/trtc_6.5.70/include
    ${PROJECT_SOURCE_DIR}/iot-explorer-c-sdk/sdk/include
    ${PROJECT_SOURCE_DIR}/iot-explorer-c-sdk/sdk/include/exports
    ${PROJECT_SOURCE_DIR}/iot-explorer-c-sdk/sdk/sdk_src/internal_inc
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/output/bin)

# 编译explorer sdk
if (${SAMPLE_ONLY})
    file(GLOB iot_c_sdk ${CMAKE_SOURCE_DIR}/output/lib/*.a)
else()
    add_subdirectory(${PROJECT_SOURCE_DIR}/iot-explorer-c-sdk)
endif()

# 编译源码
file(GLOB src_qrcode ${CMAKE_CURRENT_SOURCE_DIR}/src/qrcode/*.c)
add_library(libqrcode STATIC ${src_qrcode})

file(GLOB src_iot_trtc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/trtc_iot/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/trtc_iot/data_template/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/trtc_iot/data_template/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/trtc_iot/device_bind/*.c
)
add_library(libiot_trtc STATIC ${src_iot_trtc})

# 设置依赖库
file(GLOB lib_trtc ${CMAKE_SOURCE_DIR}/trtc_sdk/trtc_6.5.70/lib/*.so)

# 编译生成可执行程序
file(GLOB src_trtc_demo ${CMAKE_CURRENT_SOURCE_DIR}/sample/*.c)
add_executable(trtc_demo ${src_trtc_demo})
target_link_libraries(trtc_demo  -Wl,--start-group libqrcode libiot_trtc TRTCEngine ${iot_c_sdk} dl m pthread z  -Wl,--end-group)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/device_info.json DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ruguo-640x360.mp4 DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/trtc_sdk/trtc_6.5.70/bin/TrtcCoreService DESTINATION ${EXECUTABLE_OUTPUT_PATH})
