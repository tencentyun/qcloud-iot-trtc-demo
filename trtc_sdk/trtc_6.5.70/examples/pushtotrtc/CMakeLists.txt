cmake_minimum_required(VERSION 2.8)
project(MSRDemo)


set(CMAKE_CXX_STANDARD 11)

# setting Flags
if(CMAKE_SYSTEM_NAME MATCHES "Linux")

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-rpath,./../../lib  -Wl,-rpath,./../../third_lib -O0 -g")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX_SDK_VER_MAIN=1 -DLINUX_SDK_VER_SUB=0 -DLINUX_SDK_VER_SVN=10000 --std=c++11 -Wl,-rpath,./../../lib  -Wl,-rpath,./../../third_lib -O0 -g")
endif()


message(STATUS " operation system is ${CMAKE_SYSTEM} name ${CMAKE_SYSTEM_NAME}")



IF(CMAKE_SYSTEM_NAME MATCHES "Linux")
link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../lib
)


link_libraries(
TRTCEngine
dl
m
pthread
z
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

set(DEMO_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usages/TencentSDKWapperMixRecord.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usages/TencentSDKWapperRecord.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usages/TencentSDKWapperPushMp4.cpp
)

add_executable(demo ${DEMO_FILES})


ENDIF()








