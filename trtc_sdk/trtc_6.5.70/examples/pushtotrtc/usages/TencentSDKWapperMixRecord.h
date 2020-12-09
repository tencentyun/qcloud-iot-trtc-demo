#ifndef __TENCENT_SDK_WAPPER_MIXRECORD_H__
#define __TENCENT_SDK_WAPPER_MIXRECORD_H__
#include <stdio.h>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>
#include <map>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <mutex>
#include <streambuf>
#include <sys/time.h>
#include <string.h>
#include "TRTCCloud.h"
#include "ITRTCMediaRecorder.h"
#include "ITRTCMediaMixer.h"


// 简单布局管理类 - 主要演示 混流组件的使用。
// 一个老师，3个学生，最左是一个老师。
// 第一个进房的作为老师，老师退出房间，录制结束。
//
// 注：该布局如果不满足您的需求，请自行设计一个。
//+-------------------------------------------------------------------------+
//|                                                                         |
//|  +--------------+    +--------------+  +-------------+  +-------------+ |
//|  |              |    |              |  |             |  |             | |
//|  |              |    |              |  |             |  |             | |
//|  |              |    |              |  |             |  |             | |
//|  |              |    |              |  |             |  |             | |
//|  |              |    |              |  |             |  |             | |
//|  |              |    |              |  |             |  |             | |
//|  +--------------+    +--------------+  +-------------+  +-------------+ |
//|                                                                         |
//+-------------------------------------------------------------------------+
class LayoutMgr{
public:
    LayoutMgr(int s_width, int s_height, int s_count, int t_width, int t_height, int v_margin, int h_margin);
    
    ~LayoutMgr();
    
    void addStudent(std::string userid);
    void delStudent(std::string userid);
    
    void setTeacher(std::string userid);
    
    std::string& getTeacher(){return teacher;}
    
    void getRegions(std::map<std::string,Region>& regions, int &canvas_width, int &canvas_height);
    
    
private:
    std::vector<std::string>              students;
    int                                 student_width;
    int                                 student_height;
    int                                 student_count;
    
    std::string                         teacher;
    int                                 teacher_width;
    int                                 teacher_height;
    int                                 v_margin;
    int                                 h_margin;
    
    int                                 canvas_width;
    int                                 canvas_height;
};



/**
 * 组合使用 ITRTCCloud 、ITRTCMediaMixer 和 ITRTCMediaRecorder 组件，实现房间成员的流的分别录制。
 *
 */
class TencentSDKWarperMixRecord
: public ITRTCCloudCallback
, public ITRTCAudioRecvCallback
, public ITRTCVideoRecvCallback
, public ITRTCMediaRecorderCallback
, public ITRTCMediaMixerCallback
{
public:
    TencentSDKWarperMixRecord(uint32_t sdkappid);
    virtual ~TencentSDKWarperMixRecord();
    
    // 开始录制
    void start(TRTCParams &params, const char* record_roomid, const char* streamname, int output, const char* dir);
    
    // 结束录制
    void stop();
    
    // 是否录制中
    // reason代表结束原因。 1. 主播全部退出房间自动结束，2进房失败退出。
    bool isRunning(int& reason);
    
protected:
    // ITRTCCloudCallback.
    void onError(TXLiteAVError errCode, const char* errMsg, void* arg) override;
    void onWarning(TXLiteAVWarning warningCode, const char* warningMsg, void* arg) override;
    void onEnterRoom(uint64_t elapsed) override;
    void onExitRoom(int reason) override;
    void onUserEnter(const char* userId) override;
    void onUserExit(const char* userId, int reason) override;
    void onFirstVideoFrame(const char* userId, uint32_t width, uint32_t height) override;
    void onFirstAudioFrame(const char* userId) override;
    void onUserVideoAvailable(const char* userId, bool available) override {};
    void onUserAudioAvailable(const char* userId, bool available) override {};
    void onConnectionLost()override {};
    void onTryToReconnect()override {};
    void onConnectionRecovery()override {};
    void onRecvSEIMsg(const char* userId, const unsigned char* data, int datalen) override;
    
    // ITRTCAudioRecvCallback
    void onRecvAudioFrame(const char* userId, TRTCAudioFrame* frame) override;
    // ITRTCVideoRecvCallback
    void onRecvVideoFrame(const char* userId, TRTCVideoStreamType streamType, TRTCVideoFrame* frame) override;
    
    // ITRTCMediaMixerCallback
    void onError(int errcode, char* errmsg) override;
    void onMixedVideoFrame(TRTCVideoFrame* frame) override;
    void onMixedAudioFrame(TRTCAudioFrame* frame) override;
    
    // ITRTCMediaRecorderCallback
    void onFinished(const char* userId, int output, const char* filepath) override;

private:
    // 更新布局信息到 混流器。
    void updateLayout();
    
private:
    // 实时音视频房间组件，一个进程最多只能有一个。需要支持多个请使用跨进程版本。
    ITRTCCloud*                          _room;
    
    // 将 _room 的多路流 混成一路
    ITRTCMediaMixer*                      _mixer;
    
    // 将 _mixer 混后的流 录制成文件。
    ITRTCMediaRecorder*                    _recorder;
    
    // 老师学生进出房间，动态管理布局。根据事先设计好的布局逻辑。
    std::shared_ptr<LayoutMgr>              _layout;
    
    // 混合后的流取个名字，mixer就很不错，最好要和房间其它用户名重合。
    std::string                          _mixname;
    
    // 应用ID
    uint32_t                            _sdkappid;
    
    // 当前房间主播人数
    uint32_t                            _anchor_count;
    
    // 退出原因
    uint32_t                            _reason;
    
    std::recursive_mutex                  _mutex;
    
    bool                               _started;
    
    bool                               _mix_audio;
    bool                               _mix_video;
    
};

#endif // __TENCENT_SDK_WAPPER_MIXRECORD_H__
