######################CONFIG BEGIN####################################
# 编译类型: release/debug
set(BUILD_TYPE "")

# 代码抽取，ON表示根据配置抽取源码到output/qcloud_iot_c_sdk目录
set(EXTRACT_SRC OFF)

# 编译工具链
set(PLATFORM 	  "linux")
 
# 是否使能设备动态注册
set(FEATURE_DEV_DYN_REG_ENABLED ON)

# 是否使能网关功能
set(FEATURE_GATEWAY_ENABLED OFF)

# 是否使能网关子设备动态绑定解绑功能
set(FEATURE_GATEWAY_DYN_BIND_SUBDEV_ENABLED OFF)

# 是否使能多线程
set(FEATURE_MULTITHREAD_ENABLED ON)

# 是否打开数据模板事件上报功能
set(FEATURE_EVENT_POST_ENABLED ON)

# 是否打开数据模板行为功能
set(FEATURE_ACTION_ENABLED ON)

# 是否打开OTA固件升级总开关
set(FEATURE_OTA_COMM_ENABLED ON)

# 是否使能资源管理
set(FEATURE_RESOURCE_UPDATE_ENABLED ON)

# 是否使能语音识别功能
set(FEATURE_ASR_ENABLED ON)

# 接入认证方式，使用证书认证：CERT；使用密钥认证：KEY
set(FEATURE_AUTH_MODE "KEY")

# 接入认证是否不使用TLS，证书方式必须选择使用TLS，密钥认证可选择不使用TLS
set(FEATURE_AUTH_WITH_NOTLS ON)

# 是否打开代码中获取设备信息功能，OFF时将从device_info.json中读取设备信息
set(FEATURE_DEBUG_DEV_INFO_USED OFF)

# 是否使用HTTPS下载固件
set(FEATURE_OTA_USE_HTTPS ON)

# 是否打开获取iot后台时间功能
set(FEATURE_SYSTEM_COMM_ENABLED OFF)

# 是否打开日志上报云端功能
set(FEATURE_LOG_UPLOAD_ENABLED OFF)

# 使用SDK AT组件实现通用TCP模组网络读写需要配置 --->begin
# 是否打开AT模组TCP功能
set(FEATURE_AT_TCP_ENABLED OFF)

# 设置AT模组设备（目前只支持esp8266，如果是其他模组请参照at_device_esp8266.c开发at_device驱动）
set(AT_DEVICE_NAME "esp8266")

# 是否打开AT模组中断接收功能
set(FEATURE_AT_UART_RECV_IRQ ON)

# 是否打开AT模组多线程功能
set(FEATURE_AT_OS_USED OFF)

# 是否打开AT模组调试功能
set(FEATURE_AT_DEBUG OFF)

# 使用SDK AT组件实现通用TCP模组网络读写需要配置 <---end

######################CONFIG END######################################

# 解析配置
option(IOT_DEBUG "Enable IOT_DEBUG" OFF)

if(${FEATURE_AUTH_MODE} STREQUAL  "KEY")
	option(AUTH_MODE_KEY "Enable AUTH_MODE_KEY" ON)
	option(AUTH_MODE_CERT "Enable AUTH_MODE_CERT" OFF)
elseif(${FEATURE_AUTH_MODE} STREQUAL  "CERT" AND ${FEATURE_AUTH_WITH_NOTLS} STREQUAL "OFF")
	option(AUTH_MODE_KEY "Enable AUTH_MODE_KEY" OFF)
	option(AUTH_MODE_CERT "Enable AUTH_MODE_CERT" ON)
	option(AUTH_WITH_NOTLS "Enable AUTH_WITH_NOTLS" OFF)
else()
	message(FATAL_ERROR "INVAILD AUTH_MODE:${FEATURE_AUTH_MODE} WITH AUTH_WITH_NOTLS:${FEATURE_AUTH_WITH_NOTLS}!")
endif()

if(${FEATURE_OTA_COMM_ENABLED} STREQUAL "ON")
	option(OTA_MQTT_CHANNEL "Enable OTA_MQTT_CHANNEL" ON)
endif()

option(DEV_DYN_REG_ENABLED "Enable DEV_DYN_REG" ${FEATURE_DEV_DYN_REG_ENABLED})
option(GATEWAY_ENABLED "Enable GATEWAY" ${FEATURE_GATEWAY_ENABLED})
option(GATEWAY_DYN_BIND_SUBDEV_ENABLED "Enable GATEWAY_DYN_BIND_SUBDEV" ${FEATURE_GATEWAY_DYN_BIND_SUBDEV_ENABLED})
option(AUTH_WITH_NOTLS "Enable AUTH_WITH_NOTLS" ${FEATURE_AUTH_WITH_NOTLS})
option(EVENT_POST_ENABLED "Enable EVENT_POST" ${FEATURE_EVENT_POST_ENABLED})
option(ACTION_ENABLED "Enable ACTION" ${FEATURE_ACTION_ENABLED})
option(AT_TCP_ENABLED "Enable AT_TCP" ${FEATURE_AT_TCP_ENABLED})
option(DEBUG_DEV_INFO_USED "Enable DEBUG_DEV_INFO_USED" ${FEATURE_DEBUG_DEV_INFO_USED})
option(SYSTEM_COMM "Enable SYSTEM_COMM" ${FEATURE_SYSTEM_COMM_ENABLED})
option(LOG_UPLOAD "Enable LOG_UPLOAD" ${FEATURE_LOG_UPLOAD_ENABLED})

if(${FEATURE_AUTH_WITH_NOTLS} STREQUAL "ON" )
	option(OTA_USE_HTTPS "Enable OTA_USE_HTTPS" OFF)
else()
	option(OTA_USE_HTTPS "Enable OTA_USE_HTTPS" ${FEATURE_OTA_USE_HTTPS})
endif()

if(${FEATURE_GATEWAY_ENABLED} STREQUAL "ON")
	option(MULTITHREAD_ENABLED "Enable MULTITHREAD" ON)
else()
	option(MULTITHREAD_ENABLED "Enable MULTITHREAD" ${FEATURE_MULTITHREAD_ENABLED})	
endif()

if(AT_TCP_ENABLED STREQUAL "ON")
	option(AT_UART_RECV_IRQ "Enable AT_UART_RECV_IRQ" ${FEATURE_AT_UART_RECV_IRQ})
	option(AT_OS_USED "Enable AT_UART_RECV_IRQ" ${FEATURE_AT_OS_USED})
	option(AT_DEBUG "Enable AT_DEBUG" ${FEATURE_AT_DEBUG})
else()
	option(AT_UART_RECV_IRQ "Enable AT_UART_RECV_IRQ" OFF)
	option(AT_OS_USED "Enable AT_UART_RECV_IRQ" OFF)
	option(AT_DEBUG "Enable AT_DEBUG" OFF)
endif()

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/sdk/tools/cmake_scripts/config.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/config.h" 
  @ONLY
)

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
					${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/exports 
					${CMAKE_CURRENT_SOURCE_DIR}/sdk/platform/at_device/esp8266 
					${CMAKE_CURRENT_SOURCE_DIR}/sdk/sdk_src/internal_inc
					${CMAKE_CURRENT_SOURCE_DIR}/sdk/external_libs/mbedtls/include)

file(GLOB src_include ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/*.h)
file(GLOB src_inc_export ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include/exports/*.h)
file(COPY ${src_include} DESTINATION ${PROJECT_SOURCE_DIR}/output/include)
file(COPY ${src_inc_export} DESTINATION ${PROJECT_SOURCE_DIR}/output/include/exports)

if(${EXTRACT_SRC} STREQUAL  "ON" AND  NOT DEFINED SAMPLE_ONLY)
	file(GLOB src_internal_inc ${CMAKE_CURRENT_SOURCE_DIR}/sdk/sdk_src/internal_inc/*.h)
	file(COPY ${src_include} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/sdk/output/qcloud_iot_c_sdk/include)
	file(COPY ${src_inc_export} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/sdk/output/qcloud_iot_c_sdk/include/exports)
	file(COPY ${src_internal_inc} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/sdk/output/qcloud_iot_c_sdk/sdk_src/internal_inc)

	if(AT_TCP_ENABLED STREQUAL "ON")
		file(GLOB src_at_inc ${CMAKE_CURRENT_SOURCE_DIR}/sdk/platform/at_device/${AT_DEVICE_NAME}/*.h)
		file(COPY ${src_at_inc} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/sdk/output/qcloud_iot_c_sdk/platform)
	endif()
endif()

# 添加源代码目录
if(NOT DEFINED SAMPLE_ONLY)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sdk/sdk_src)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sdk/platform)
	add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/sdk/external_libs/mbedtls)
endif()

# 证书及设备信息
if(NOT DEFINED SAMPLE_ONLY)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sdk/certs DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/sdk/output/${BUILD_TYPE}/bin)
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sdk/device_info.json DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/sdk/output/${BUILD_TYPE}/bin)
endif()

# sdk 库
if(${FEATURE_AUTH_WITH_NOTLS} STREQUAL "ON" )
	set(iot_c_sdk iot_sdk iot_platform CACHE INTERNAL "iot c sdk lib")
else()
	set(iot_c_sdk iot_sdk iot_platform mbedtls mbedx509 mbedcrypto  CACHE INTERNAL "iot c sdk lib")
endif()
