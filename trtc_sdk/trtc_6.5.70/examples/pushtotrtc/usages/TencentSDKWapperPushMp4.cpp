#include "TencentSDKWapperPushMp4.h"

namespace  {
    constexpr int PATH_INVALID = 0;
    constexpr int TYPE_MAIN = 1;
    constexpr int TYPE_AUX = 2;
    
    // 自定义音视频数据加密开关。确保对端采用了相同算法和秘钥的自定义解密处理。否则对端播放黑屏。没有声音。
    constexpr bool ENABLE_CUSTOM_ENCRYPT = false;
}

#pragma mark - MP4Sender.
Mp4Sender::Mp4Sender(std::weak_ptr<TencentSDKWarperPushMp4> room)
: _mp4_reader(nullptr){
    _room = room;
    _info.file_path = "";
    _info.type = PATH_INVALID; // 表示内部是无效状态，不会推流。
    _info.video_width = 0;
    _info.video_height = 0;
    _info.video_fps = 0;
}

Mp4Sender::~Mp4Sender(){
    if (_mp4_reader) {
        _mp4_reader->setNotify(nullptr);
        _mp4_reader->setDataListener(nullptr);
        destroyMSRInstance(_mp4_reader);
        _mp4_reader = nullptr;
    }
}

void Mp4Sender::setPath(std::string &filepath, int type){
    // 路径为空，就啥也不用干
    if (filepath.empty()) {
        return;
    }else{
        _info.file_path = filepath;
        _info.type = type;
    }
}
void Mp4Sender::startPush(){
    if (_info.type  == 0) {
        return;
    }
    if (_mp4_reader) {
        _mp4_reader->setNotify(nullptr);
        _mp4_reader->setDataListener(nullptr);
        destroyMSRInstance(_mp4_reader);
        _mp4_reader = nullptr;
    }
    _mp4_reader = createMSRInstance();
    _mp4_reader->setDataListener(this);
    _mp4_reader->setNotify(this);
    TRTCStreamParam param;
    param.isLoop = true;
    _mp4_reader->setStreamParam(&param);
    _mp4_reader->start(_info.file_path.c_str());
}

void Mp4Sender::stopPush(){
    if (_info.type == 0) {
        return;
    }
    
    if (_mp4_reader) {
        _mp4_reader->setNotify(nullptr);
        _mp4_reader->setDataListener(nullptr);
        destroyMSRInstance(_mp4_reader);
        _mp4_reader = nullptr;
    }
}

std::string Mp4Sender::getEventMsgByCode(int event){
    switch (event) {
        case EVENT_MEDIAPLAY_PLAY_SUCCESS:
            return "EVENT_MEDIAPLAY_PLAY_SUCCESS";
        case EVENT_MEDIAPLAY_MEDIA_STOP:
            return "EVENT_MEDIAPLAY_MEDIA_STOP";
        case EVENT_MEDIAPLAY_MEDIA_PAUSE:
            return "EVENT_MEDIAPLAY_MEDIA_PAUSE";
        case EVENT_MEDIAPLAY_MEDIA_RESUME:
            return "EVENT_MEDIAPLAY_MEDIA_RESUME";
        case EVENT_MEDIAPLAY_ERROR_PLAY:
            return "EVENT_MEDIAPLAY_ERROR_PLAY";
        case EVENT_MEDIAPLAY_ERROR_STREAM_FORMAT:
            return "EVENT_MEDIAPLAY_ERROR_STREAM_FORMAT";
        case EVENT_MEDIAPLAY_ERROR_MEDIE_INFO:
            return "EVENT_MEDIAPLAY_ERROR_MEDIE_INFO";
        default:
            break;
    }
    return "unknown";
}

#pragma mark - IMediaStreamDataListener
void Mp4Sender::onMediaStreamAudioDecode(TRTCDecodeAudioFrame *frame){
    auto room = _room.lock();
    if (room && _info.type == TYPE_MAIN) {
        TRTCAudioFrame audioframe;
        audioframe.data = (uint8_t*)frame->data;
        audioframe.length = frame->length;
        audioframe.sampleRate = frame->sample;
        audioframe.channel = frame->channels;
        audioframe.timestamp = frame->timestamp;
        audioframe.audioFormat = TRTCAudioFrameFormat::TRTCAudioFrameFormat_PCM;
        
        {
            room->getRoom()->sendCustomAudioData(&audioframe);
        }
    }
}

void Mp4Sender::onMediaStreamVideoDecode(TRTCDecodeVideoFrame *frame){
    auto room = _room.lock();
    if (room) {
        TRTCVideoFrame videoframe;
        videoframe.data = (uint8_t*)frame->data;
        videoframe.bufferType = TRTCVideoBufferType::TRTCVideoBufferType_Buffer;
        videoframe.length = frame->length;
        videoframe.rotation = TRTCVideoRotation::TRTCVideoRotation0;
        videoframe.width = frame->width;
        videoframe.height = frame->height;
        videoframe.timestamp = frame->timestamp;
        videoframe.videoFormat = TRTCVideoFrameFormat::TRTCVideoFrameFormat_YUVI420;
        if (_info.video_width != frame->width || _info.video_height != frame->height) {
            TRTCVideoEncParam param;
            param.videoFps = _info.video_fps;
            //视频码率，单位kbps，这里用一个经验公式。可以参考TRTCCloudDef.h的建议码率填写。
            param.videoBitrate = frame->width * frame->height * 1.7 / 1000;
            param.encType = (TRTCVideoEncodeType)TRTCVideoEncodeType_H264;

            if (_info.type == TYPE_MAIN) {
                // 设置主流编码参数。
                room->getRoom()->setVideoEncoderParam(param);
            }else if (_info.type == TYPE_AUX) {
                // 设置辅流编码参数。
                room->getRoom()->setAuxVideoEncoderParam(param);
            }
            
            _info.video_height = frame->height;
            _info.video_width = frame->width;
        }

        if (_info.type == TYPE_MAIN) {
            // 发送主流视频
            room->getRoom()->sendCustomVideoData(&videoframe);
        }else if (_info.type == TYPE_AUX) {
            // 发送辅流视频
            room->getRoom()->sendAuxVideoData(&videoframe);
        }

    }
}

#pragma mark - IMediaPlayNotifyListener
void Mp4Sender::onMediaPlayerNotifyEvent(int event, void *param){
    printf("\n --------------  event %s(%d) --------------\n", getEventMsgByCode(event).c_str(), event);
    auto room = _room.lock();
    if (room) {
        if (event == EVENT_MEDIAPLAY_PLAY_SUCCESS) {
            if (_mp4_reader) {
                TRTCStreamInfo info;
                _mp4_reader->getFileInfo(&info);
                printf("--- duration %llu  sduration %s fps %d video width %d video height %d --\n",
                       info.duration,
                       info.str_duration,
                       info.videoFps,
                       info.videoWidth,
                       info.videoheight);
                
                TRTCVideoEncParam param;
                _info.video_fps = info.videoFps;
                _info.video_height = info.videoheight;
                _info.video_width = info.videoWidth;
                param.videoFps = info.videoFps;
                //视频码率，单位kbps，这里用一个经验公式。可以参考TRTCCloudDef.h的建议码率填写。
                param.videoBitrate = info.videoWidth * info.videoheight * 1.7 / 1000;
                param.encType = TRTCVideoEncodeType_H264;
                if (_info.type == TYPE_MAIN) {
                    // 设置主流编码参数。
                    room->getRoom()->setVideoEncoderParam(param);
                }else if (_info.type == TYPE_AUX) {
                    // 设置辅流编码参数。
                    room->getRoom()->setAuxVideoEncoderParam(param);
                }
            }
        }
    }
}

#pragma mark - TencentSDKWapperPushMp4.
namespace  {
    constexpr int REASON_FAIL = 2;
    constexpr int REASON_OK = 1;
    constexpr int REASON_UNKNOWN = 0;
};

TencentSDKWarperPushMp4::TencentSDKWarperPushMp4(uint32_t sdkappid)
:  _sdkappid(sdkappid)
, _started(false)
, _anchor_count(0)
, _reason(REASON_UNKNOWN)
, _tencentEngine(nullptr)
{
    _tencentEngine = createInstance(_sdkappid);
    _tencentEngine->setCallback(this);
}

TencentSDKWarperPushMp4::~TencentSDKWarperPushMp4() {
    _tencentEngine->setCallback(nullptr);
    destroyInstance(_tencentEngine);
    _tencentEngine = NULL;
}

void TencentSDKWarperPushMp4::start(TRTCParams &params, const char* main_mp4, const char* aux_mp4) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_started == true) {
        return;
    }
    _started = true;
    
    _sender_main.reset(new Mp4Sender(shared_from_this()));
    std::string path_main = main_mp4;
    _sender_main->setPath(path_main, TYPE_MAIN);
    
    _sender_aux.reset(new Mp4Sender(shared_from_this()));
    std::string path_aux = aux_mp4;
    _sender_aux->setPath(path_aux, TYPE_AUX);
    
    if (_tencentEngine) {
        _tencentEngine->enterRoom(params, TRTCAppScene::TRTCAppSceneVideoCall);
    }
}

void TencentSDKWarperPushMp4::stop() {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_started == false) {
        return;
    }
    _started = false;
    _sender_main->stopPush();
    _sender_aux->stopPush();
    if (_tencentEngine) {
        _tencentEngine->exitRoom();
    }
}

bool TencentSDKWarperPushMp4::isRunning(int &reason){
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_reason == REASON_FAIL) {
        return false;
    }
    
    if (_reason == REASON_OK && _anchor_count == 0) {
        return false;
    }
    
    return true;
}

void TencentSDKWarperPushMp4::sendSEIMsg(std::string data){
    if (_tencentEngine) {
        if(_tencentEngine->sendSEIMsg((const unsigned char*)data.c_str(), data.length(), 1)){
            printf("send sei msg  success!!!\n");
        }else{
            printf("send sei msg failed!!!\n");
        }
    }
}

void TencentSDKWarperPushMp4::sendCmdMsg(std::string msg){
    if (_tencentEngine) {
        if(_tencentEngine->sendCustomCmdMsg(1, (const unsigned char*)msg.c_str(), msg.length(), true, true)){
            printf("send custom msg sucess!\n");
        }else{
            printf("send custom msg failed!\n");
        }
    }
}



#pragma mark - ITRTCCloudCallback
void TencentSDKWarperPushMp4::onRecvSEIMsg(const char *userId, const unsigned char *data, int datalen){
    printf("onRecvSEIMsg  userid %s  data %s \n", userId, data);
}

void TencentSDKWarperPushMp4::onError(TXLiteAVError errCode, const char *errMsg, void *arg ) {
}

void TencentSDKWarperPushMp4::onWarning(TXLiteAVWarning warningCode, const char *warningMsg, void *arg) {
}

void TencentSDKWarperPushMp4::onEnterRoom(uint64_t elapsed) {
    
    // 注册回调
    if (ENABLE_CUSTOM_ENCRYPT){
        _tencentEngine->setEncryptionCallback(this);
    }
    
    // 进房成功后推流。
    if (_sender_main) {
        _sender_main->startPush();
    }
    
    if (_sender_aux) {
        _sender_aux->startPush();
    }
}

void TencentSDKWarperPushMp4::onExitRoom(int reason) {
    
}

void TencentSDKWarperPushMp4::onUserEnter(const char *userId) {
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        ++_anchor_count;
        _reason = REASON_OK;
    }
// 订阅远端用户 userId 的音频数据
//    if (_tencentEngine) {
//        _tencentEngine->setRemoteAudioRecvCallback(userId,TRTCAudioFrameFormat::TRTCAudioFrameFormat_PCM, this);
//    }
}

void TencentSDKWarperPushMp4::onUserExit(const char *userId, int reason) {
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        --_anchor_count;
    }
// 取消订阅远端用户 userId 的音频数据
//    if (_tencentEngine) {
//        _tencentEngine->setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_Unknown, nullptr);
//    }
}

void TencentSDKWarperPushMp4::onFirstAudioFrame(const char *userId) {
}

void TencentSDKWarperPushMp4::onFirstVideoFrame(const char *userId, uint32_t width, uint32_t height) {
}

#pragma mark - ITRTCAudioRecvCallback
void TencentSDKWarperPushMp4::onRecvAudioFrame(const char *userId, TRTCAudioFrame *frame) {
}

#pragma mark - ITRTCVideoRecvCallback
void TencentSDKWarperPushMp4::onRecvVideoFrame(const char *userId, TRTCVideoStreamType streamType, TRTCVideoFrame *frame) {
}

#pragma mark - ITRTCEncryptionCallback
bool TencentSDKWarperPushMp4::onVideoFrameEncrypt(TRTCVideoStreamType streamType, TRTCCustomEncryptionData *data) {
    // 必须是同步进行加密。
    // data 的内存由SDK负责申请和释放的。
    // 针对主视频流进行加密，加密算法 按位取反。
    printf("encrypt video len:%d\n", data->unEncryptedDataLength);
    if (streamType == TRTCVideoStreamType::TRTCVideoStreamTypeBig){
        for (int i = 0; i < data->unEncryptedDataLength; i ++) {
            data->EncryptedData[i] = ~data->unEncryptedData[i];
        }
    }
    // 很重要，加密后的数据的长度。
    data->EncryptedDataLength = data->unEncryptedDataLength;
    return true;
}

bool TencentSDKWarperPushMp4::onAudioFrameEncrypt(TRTCCustomEncryptionData *data){
    // 必须是同步进行加密。
    printf("encrypt audio len:%d\n", data->unEncryptedDataLength);
    for (int i = 0; i < data->unEncryptedDataLength; i ++) {
        data->EncryptedData[i] = ~data->unEncryptedData[i];
    }
    data->EncryptedDataLength = data->unEncryptedDataLength;
    return true;
}


