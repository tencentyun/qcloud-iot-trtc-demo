//
// 功能说明：
//   SWIG 工具可以将c++接口形式的TRTCEngine转成其它语言 java 和 go 的接口封装。
//
// 2019-10-15 @ arvinwu

%module(directors="1") trtcengine

// 虚类导出，可以用于目标语言的类继承。
%feature("director") ITRTCCloudCallback;
%feature("director") ITRTCVideoRecvCallback;
%feature("director") ITRTCAudioRecvCallback;
%feature("director") ITRTCLogCallback;
%feature("director") ITRTCMediaRecorderCallback;
%feature("director") ITRTCMediaMixerCallback;

// "%{" 和 “}%” 的内容原样输出到转换后的 c++ 文件中
%{

#include <string>
#include <stdint.h>
#include "../include/TRTCCloud.h"
#include "../include/TRTCCloudCallback.h"
#include "../include/ITRTCMediaMixer.h"
#include "../include/ITRTCMediaRecorder.h"
#include "../include/TRTCCloudDef.h"
#include "../include/TXLiteAVCode.h"

%}

// swig 解析和转换
%include "std_string.i"
%include "stdint.i"
%include "../include/TRTCCloud.h"
%include "../include/TRTCCloudCallback.h"
%include "../include/ITRTCMediaMixer.h"
%include "../include/ITRTCMediaRecorder.h"
%include "../include/TRTCCloudDef.h"
%include "../include/TXLiteAVCode.h"

// 重命名接口首字母大写
%rename("%(firstuppercase)s", %$isfunction) "";

