#ifndef __TENCENT_SDK_WAPPER_PUSHMP4_H__
#define __TENCENT_SDK_WAPPER_PUSHMP4_H__

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
#include "TRTCMediaStreamReader.h"

// MP4 文件信息。
typedef struct mp4fileinfo{
    std::string   file_path;
    int          video_width;
    int          video_height;
    int          video_fps;
    int          type;// 1 for main 2 for aux.
}sFileInfo;

class TencentSDKWarperPushMp4;
// 负责解析MP4 并发送数据。
class Mp4Sender
: public IMediaStreamDataListener
, public IMediaPlayNotifyListener {
public:
    Mp4Sender(std::weak_ptr<TencentSDKWarperPushMp4> room);
    virtual ~Mp4Sender();
    
    // type 1 for main, 2 for aux.
    void setPath(std::string& filepath, int type);
    
    // 开始本地MP4推流，调用前确认已经通过 setPushFile 设置了文件路径。
    void startPush();
    
    // 停止推流。
    void stopPush();
private:
    void onMediaStreamAudioDecode(TRTCDecodeAudioFrame *frame) override;
    void onMediaStreamVideoDecode(TRTCDecodeVideoFrame *frame) override;
    void onMediaPlayerNotifyEvent(int event, void *param) override;
    
private:    
    // 错误信息翻译。
    std::string getEventMsgByCode(int event);
    
private:
    std::weak_ptr<TencentSDKWarperPushMp4>              _room;
    ITRTCMediaStreamReader*                         _mp4_reader;
    sFileInfo                                   _info;
};


/**
 * 腾讯SDK封装。推本地Mp4文件。
 *
 */
class TencentSDKWarperPushMp4
: public ITRTCCloudCallback
, public ITRTCAudioRecvCallback
, public ITRTCVideoRecvCallback
, public ITRTCEncryptionCallback
, public std::enable_shared_from_this<TencentSDKWarperPushMp4>{
public:
    TencentSDKWarperPushMp4(uint32_t sdkappid);
    virtual ~TencentSDKWarperPushMp4();
    
    // 开始
    void start(TRTCParams &params, const char* main_mp4, const char* aux_mp4);
    
    // 结束
    void stop();
    
    bool isRunning(int& reason);
    
    // 发送SEI消息，一般用于直播答题场景。
    void sendSEIMsg(std::string data);
    
    // 发送自定义广播消息，极简版的IM，频率和量有限制。
    void sendCmdMsg(std::string msg);
    
    // 给Mp4Sender 用。
    ITRTCCloud* getRoom(){return _tencentEngine;}
    
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
    
    // ITRTCEncryptionCallback
    bool onAudioFrameEncrypt(TRTCCustomEncryptionData* data) override;
    bool onVideoFrameEncrypt(TRTCVideoStreamType streamType, TRTCCustomEncryptionData* data) override;
    
private:
    // 实时音视频房间组件，一个进程最多只能有一个。需要支持多个请使用跨进程版本。
    ITRTCCloud*                          _tencentEngine;
    
    // 主流MP4解析用
    std::shared_ptr<Mp4Sender>               _sender_main;
    
    // 辅流MP4解析用
    std::shared_ptr<Mp4Sender>              _sender_aux;
    
    uint32_t                            _sdkappid;
    // 退出原因
    uint32_t                            _reason;
    
    std::recursive_mutex                  _mutex;
    
    bool                               _started;
    
    int                                 _anchor_count;
    
};
#endif //__TENCENT_SDK_WAPPER_PUSHMP4_H__
