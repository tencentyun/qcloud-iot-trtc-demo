#ifndef _TENCENT_SDK_WAPPER_RECORD_H__
#define _TENCENT_SDK_WAPPER_RECORD_H__

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

typedef enum eRecMode{
    MODE_DIRECT_REC = 0,    // 直接录模式
    MODE_MIX_REC = 1        // 混流+录制模式，不受禁音，禁画和多分辨率的影响录制文件。
} RecMode;

/**
 * 负责录制单流。有两种模式，
 * 1。直接录，数据直接交给 ITRTCMediaRecorder 完成录制写文件。
 * 2。混+录，数据先交给 ITRTCMediaMixer 做缩放补音和补画的处理，然后交给录制模块 ITRTCMediaRecorder。
 */
class StreamRecorder
: public ITRTCMediaMixerCallback {
public:
    
    StreamRecorder(RecMode mode, bool needaudio, bool needvideo, ITRTCMediaRecorder* rec);
    ~StreamRecorder();
    
    /**
     * MODE_DIRECT_REC 模式下 用于指定替补帧的颜色，视频输出帧率。
     */
    void setInfo(int color, int fps);
    
    /**
     * 处理要录制的音频帧，直接转给_recorder处理。
     */
    void addAudioFrame(const char* userId, TRTCAudioFrame* frame);
    
    /**
     * 处理要录制的视频帧，MODE_MIX_REC 模式下数据就转给_mixer处理。
     * frame 为nullptr 表示，需要插补指定的画面。
     */
    void addVideoFrame(const char* userId, TRTCVideoStreamType streamType, TRTCVideoFrame* frame);
    
    /**
     * 停止当前流的录制。
     */
    void stop();
    
protected:
    void onMixedAudioFrame(TRTCAudioFrame* frame) override;
    void onMixedVideoFrame(TRTCVideoFrame* frame) override;
    void onError(int errcode, char* errmsg) override;
    
private:
    /**
     * 生成期望颜色的和指定分辨率的视频帧。
     * color  期望的颜色 RGB格式 0xFFFFFF
     * width  宽度
     * height 高度
     * frame  生成的frame
     */
    void getBackupPic(int color, int width, int height, TRTCVideoFrame& frame);
    
private:
    /**
     * 初始化时，从外部传入。注意生命期管理和判空。
     */
    ITRTCMediaRecorder*         _recorder;
    
    /**
     * 混流录制模式 MODE_MIX_REC 下，作用是补帧。
     */
    ITRTCMediaMixer*           _mixer;
    
    /**
     * 单流录制的模式，MODE_DIRECT_REC，直接录制，数据完全依赖外部输入，不会补帧。MODE_MIX_REC 补帧录制。
     */
    RecMode                     _mode;
    
    /**
     * 最后一帧视频帧的状态，包括时间戳，宽，高。由于要补的帧的生成。
     */
    uint64_t                 _last_video_ts; // 补视频帧时会用到
    int                     _last_width;
    int                     _last_height;
    
    /**
     * 要补帧的颜色，0xFFFFFF（RGB）表示白色。
     */
    int                     _color;
    int                     _fps;
    
    /**
     * 用于音视频数据过滤。
     */
    int                     _stream_type;
    std::string              _userid;
    
    std::mutex               _mutex;
    
    /**
     * 支持纯音频，纯视频和音视频等模式的录制。
     */
    bool                    _need_audio;
    bool                    _need_video;
};


/**
 *  单流录制，默认是补帧录制方式。
 */
using CMapStreamRecorders = std::map<std::string, std::shared_ptr<StreamRecorder>>;

class TencentSDKWarperRecord
: public ITRTCCloudCallback
, public ITRTCAudioRecvCallback
, public ITRTCVideoRecvCallback
, public ITRTCDecryptionCallback
, public ITRTCMediaRecorderCallback
{
public:
    TencentSDKWarperRecord(uint32_t sdkappid);
    virtual ~TencentSDKWarperRecord();
    
    /**
     * 启动录制
     * params   房间参数。
     * output   需要录制的文件类型，支持同时录纯音频，纯视频和音视频文件。参见 OutputFileType 定义
     * dir      录制的文件存放的路径。
     */
    void start(TRTCParams &params, const char* record_roomid, int output, const char* dir);
    
    /**
     * 结束房间录制，停止所有正在录制的流。
     */
    void stop();
    
    /**
     * 当前录制状态
     *  reason 若返回false，reason存放导致false的原因。
     *   1. 主播全部退出房间自动结束，
     *   2. 进房失败退出
     */
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
    
    // ITRTCMediaRecorderCallback
    void onFinished(const char* userId, int output, const char* filepath) override;
    
    // ITRTCDecryptionCallback
    bool onAudioFrameDecrypt(const char* userId, TRTCCustomEncryptionData* data) override;
    bool onVideoFrameDecrypt(const char* userId, TRTCVideoStreamType streamType, TRTCCustomEncryptionData* data) override;
    
private:
    /**
     * SDK 提供的房间组件
     */
    ITRTCCloud*                          _room;
    
    /**
     *  SDK 提供的录制组件
     */
    ITRTCMediaRecorder*                    _recorder;
    
    /**
     * 客户sdkappid，ITRTCMediaRecorder 需要用到。
     */
    uint32_t                            _sdkappid;
    
    /**
     * 房间主播人数（不包括自己）
     */
    uint32_t                            _anchor_count;
    
    /**
     * 退房原因
     */
    uint32_t                            _reason;
    
    std::recursive_mutex                  _mutex;
    
    /**
     * 组件启动状态，做防重入。start 或 stop 连续调用两次，第二次直接返回失败。
     */
    bool                                _started;
    
    /**
     * 录制输出要求。
     */
    bool                                _need_audio;
    bool                                _need_video;
    
    /**
     * 单流录制器。
     */
    CMapStreamRecorders                    _stream_recorders;
    
    /**
     * 录制模式，参见，默认是补帧录制。
     */
    RecMode                                 _mode;
};


#endif //_TENCENT_SDK_WAPPER_RECORD_H__
