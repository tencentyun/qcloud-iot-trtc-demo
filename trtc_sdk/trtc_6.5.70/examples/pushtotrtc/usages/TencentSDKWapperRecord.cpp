#include "TencentSDKWapperRecord.h"
#include <cmath>


namespace {
    // 默认关闭，若打开，确保对端发送采用了相同算法和秘钥的自定义加密处理。否则本端收不到音视频回调。
    constexpr bool ENABLE_CUSTOM_DECRYPT = false;
}

#pragma mark - StreamRecorder
StreamRecorder::StreamRecorder(RecMode mode, bool needaudio, bool needvideo, ITRTCMediaRecorder* rec)
: _recorder(rec)
, _mixer(nullptr)
, _mode(mode)
, _last_video_ts(0)
, _last_width(0)
, _last_height(0)
, _color(0)
, _fps(0)
, _stream_type(-1)
, _userid("")
, _need_audio(needaudio)
, _need_video(needvideo){
    printf("【StreamRec】init mode %s audio %s video %s rec %p\n",mode == MODE_MIX_REC?"mix":"no-mix", needaudio?"yes":"no", needvideo?"yes":"no", rec);
}

StreamRecorder::~StreamRecorder(){
    std::lock_guard<std::mutex> lock(_mutex);
    if (_mixer) {
        _mixer->setCallback(nullptr);
        destroyMediaMixer(_mixer);
        _mixer = nullptr;
    }
}

void StreamRecorder::setInfo(int color, int fps){
    _color = color;
    _fps = fps;
}

void StreamRecorder::stop(){
    std::lock_guard<std::mutex> lock(_mutex);
    if (_mixer) {
        _mixer->stop();
    }
    
    if (_recorder && !_userid.empty()) {
        _recorder->stopOne(_userid.c_str());
    }
}

void StreamRecorder::addAudioFrame(const char *userId, TRTCAudioFrame *frame){
    if (frame == nullptr) {
        return;
    }
    
    if (userId == nullptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(_mutex);
    if (_userid.empty()) {
        _userid = userId;
    }
    
    if (_userid != userId) {
        printf("【StreamRec】E addAudioFrame inner userid %s but input is %s\n",_userid.c_str(), userId);
        return;
    }
    
    if (MODE_DIRECT_REC == _mode) {
        if (_recorder && _need_audio) {
            _recorder->addAudioFrame(userId, frame);
        }
    }else{
        if ((_need_audio && _need_video == false) && _mixer == nullptr) {
            _mixer = createMediaMixer();
            _mixer->setCallback(this);
            // 虽然不用混图，但是这个设置还是需要的。填写合法参数即可，虽然不起作用。
            _mixer->setCanvas(_fps, 16, 16, 0xFFFFFF);
            Region region;
            region.width = 16;
            region.height = 16;
            region.offset_x = 0;
            region.offset_y = 0;
            region.zOrder = 1;
            region.bgcolor = 0xFFFFFF;
            region.option = REGION_OPTION_FILLMODE_FULL;
            
            _mixer->setRegion(userId, &region);
            _mixer->applyRegions();
            _mixer->start(_need_audio, _need_video);
        }
        
        if (_mixer && _need_audio) {
            _mixer->addAudioFrame(userId, frame);
        }
    }
}

void StreamRecorder::addVideoFrame(const char *userId, TRTCVideoStreamType streamType, TRTCVideoFrame *frame){
    if (userId == nullptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(_mutex);
    if (_userid.empty()) {
        _userid = userId;
    }
    
    if (_stream_type == -1) {
        _stream_type = streamType;
    }
    
    if (_userid != userId) {
        printf("【StreamRec】E addVideoFrame inner userid %s but input is %s\n",_userid.c_str(), userId);
        return;
    }
    
    if (_stream_type != streamType) {
        printf("【StreamRec】E addVideoFrame inner streamtype %d but input streamtype %d\n", _stream_type, streamType);
        return;
    }
    
    
    if (MODE_DIRECT_REC == _mode) {
        if (frame == nullptr) {
            return;
        }
        
        if (_recorder && _need_video) {
            _recorder->addVideoFrame(userId, streamType, frame);
        }
    }else{
        if (_need_video && _mixer == nullptr && frame) {
            _mixer = createMediaMixer();
            _mixer->setCallback(this);
            _mixer->setCanvas(_fps, frame->width, frame->height, 0xFFFFFF);
            Region region;
            region.width = frame->width;
            region.height = frame->height;
            region.offset_x = 0;
            region.offset_y = 0;
            region.zOrder = 1;
            region.bgcolor = 0xFFFFFF;
            region.option = REGION_OPTION_FILLMODE_FULL;
            
            _mixer->setRegion(userId, &region);
            _mixer->applyRegions();
            _mixer->start(_need_audio, _need_video);
        }
    
        if (_mixer && _need_video) {
            if (frame == nullptr) { // 需要补片
                printf("【StreamRec】addVideoFrame add a packup frame!\n");
                TRTCVideoFrame shim;
                getBackupPic(_color, _last_width, _last_height, shim);
                shim.timestamp = _last_video_ts + 300; // 300ms 延迟。
                _mixer->addVideoFrame(userId, &shim);
                delete[] shim.data;
            }else{
                _mixer->addVideoFrame(userId, frame);
                _last_height = frame->height;
                _last_width = frame->width;
                _last_video_ts = frame->timestamp;
            }
        }
    }
    
}

void StreamRecorder::getBackupPic(int color, int width, int height, TRTCVideoFrame &frame){
    // 生成由 color 指定颜色的视频帧。
    uint8_t R = (color & 0x00FF0000) >> 16;
    uint8_t G = (color & 0x0000FF00) >> 8;
    uint8_t B = (color & 0x000000FF);
    
    uint8_t Y = round(0.299*R + 0.587*G + 0.114*B);
    uint8_t U = round(-0.169*R -0.331*G + 0.5*B + 128);
    uint8_t V = round(0.5*R - 0.419*G - 0.081*B + 128);
    
    if (frame.data != nullptr) {
        return;
    }
    
    frame.data = new uint8_t[width*height*3/2];
    frame.length = width*height*3/2;
    memset(frame.data, Y, width*height);
    memset(frame.data + width*height, U, width*height/4);
    memset(frame.data + width*height + width*height/4, V, width*height/4);
    
    frame.width = width;
    frame.height = height;
    frame.bufferType = TRTCVideoBufferType::TRTCVideoBufferType_Buffer;
    frame.rotation = TRTCVideoRotation::TRTCVideoRotation0;
    frame.textureId = 0;
    frame.videoFormat = TRTCVideoFrameFormat_YUVI420;
}

void StreamRecorder::onMixedAudioFrame(TRTCAudioFrame *frame){
    if (_recorder) {
        _recorder->addAudioFrame(_userid.c_str(), frame);
    }
    
}

void StreamRecorder::onMixedVideoFrame(TRTCVideoFrame *frame){
    if (_recorder) {
        _recorder->addVideoFrame(_userid.c_str(), (TRTCVideoStreamType)_stream_type, frame);
    }
}

void StreamRecorder::onError(int errcode, char *errmsg) {
    
}


#pragma mark - TencentSDKWarperRecord
namespace  {
    constexpr int REASON_FAIL = 2;
    constexpr int REASON_OK = 1;
    constexpr int REASON_UNKNOWN = 0;
};



TencentSDKWarperRecord::TencentSDKWarperRecord(uint32_t sdkappid)
: _sdkappid(sdkappid)
, _anchor_count(0)
, _reason(REASON_UNKNOWN)
, _started(false)
, _need_audio(false)
, _need_video(false)
, _mode(MODE_MIX_REC)
{
    // 初始化房间组件
    _room = createInstance(_sdkappid);
    _room->setCallback(this);
    
    // 初始化录制组件
    _recorder = createMediaRecorder();
    _recorder->setCallback(this);
}

TencentSDKWarperRecord::~TencentSDKWarperRecord() {
    if (_room) {
        _room->setCallback(nullptr);
        destroyInstance(_room);
        _room = NULL;
    }

    if (_recorder) {
        _recorder->setCallback(nullptr);
        destroyMediaRecorder(_recorder);
        _recorder = NULL;
    }
    
    if (_stream_recorders.size()) {
        _stream_recorders.clear();
    }
}

void TencentSDKWarperRecord::start(TRTCParams &params, const char* record_roomid, int output, const char *dir) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_started == true) {
        return;
    }
    _started = true;
    
    if ( (output & OUTFILE_TYPE_AUDIO) || (output & OUTFILE_TYPE_AUDIO_VIDEO)) {
        _need_audio = true;
    }
    
    if ( (output & OUTFILE_TYPE_VIDEO) || (output & OUTFILE_TYPE_AUDIO_VIDEO)) {
        _need_video = true;
    }
    
    
    if (_room) {
        _room->enterRoom(params, TRTCAppScene::TRTCAppSceneVideoCall);
    }
    
    if (_recorder) {
        _recorder->setParam(_sdkappid, record_roomid, (char*)params.userId.buffer, output, dir);
    }
}

void TencentSDKWarperRecord::stop() {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_started == false) {
        return;
    }
    _started = false;
    
    if (_room) {
        _room->exitRoom();
    }
    
    for (auto rec: _stream_recorders) {
        rec.second->stop();
    }
    
    _stream_recorders.clear();
}

bool TencentSDKWarperRecord::isRunning(int &reason) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_reason == REASON_FAIL) {
        return false;
    }
    
    if (_reason == REASON_OK && _anchor_count == 0) {
        return false;
    }
    
    return true;
}

#pragma mark - ITRTCCloudCallback
void TencentSDKWarperRecord::onRecvSEIMsg(const char *userId, const unsigned char *data, int datalen){
}

void TencentSDKWarperRecord::onError(TXLiteAVError errCode, const char *errMsg, void *arg ) {
    // ITRTCCloud 组件内部报错，说明功能不可用了。建议打印错误信息，然后退出房间。
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    _reason = REASON_FAIL;
}

void TencentSDKWarperRecord::onWarning(TXLiteAVWarning warningCode, const char *warningMsg, void *arg) {
}

void TencentSDKWarperRecord::onEnterRoom(uint64_t elapsed) {
    if (ENABLE_CUSTOM_DECRYPT) {
        _room->setDecryptionCallback(this);
    }
}

void TencentSDKWarperRecord::onExitRoom(int reason) {
    
}

void TencentSDKWarperRecord::onUserEnter(const char *userId) {
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        ++_anchor_count;
        _reason = REASON_OK;
        if (_started == false){
            // 未开始或已近结束中，不处理后续流程。
            return;
        }
    }
    
    // 如果关心 userId 这个用户的流，需要主动设置数据回调，不关心的话就不用设置，数据就不会回调上来
    // example:
    // if (isInBlackList(userId)){
    //     return;
    // }
    if (_room) {
        if (_need_audio) {
            _room->setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_PCM, this);
        }
        if (_need_video) {
            _room->setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_YUVI420, this);
        }
    }
}

void TencentSDKWarperRecord::onUserExit(const char *userId, int reason) {
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        --_anchor_count;
    }
    
    // 不关心 userid 的 音视频数据了，主动设置一下是个好习惯。
    if (_room) {
        if (_need_audio) {
            _room->setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_Unknown, nullptr);
        }
        if (_need_video) {
            _room->setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_Unknown, nullptr);
        }
    }
    
    if (_mode == MODE_MIX_REC && _need_video) {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto rec = _stream_recorders.find(userId);
        if (rec != _stream_recorders.end()) {
            if (rec->second) {
                // 补一帧。
                rec->second->addVideoFrame(userId, TRTCVideoStreamType::TRTCVideoStreamTypeBig, nullptr);
            }
        }
    }
}

void TencentSDKWarperRecord::onFirstAudioFrame(const char *userId) {
}

void TencentSDKWarperRecord::onFirstVideoFrame(const char *userId, uint32_t width, uint32_t height) {
}

bool TencentSDKWarperRecord::onVideoFrameDecrypt(const char *userId, TRTCVideoStreamType streamType, TRTCCustomEncryptionData *data){
    // 必须是同步进行解密。
    // data 的内存由SDK负责申请和释放的。
    // 针对主视频流进行解密，解密算法 按位取反。
    printf("video decrypt len:%d!\n", data->EncryptedDataLength);
    if (streamType == TRTCVideoStreamType::TRTCVideoStreamTypeBig){
        for (int i = 0; i < data->EncryptedDataLength; i ++) {
            data->unEncryptedData[i] = ~data->EncryptedData[i];
        }
    }
    // 很重要，加密后的数据的长度。
    data->unEncryptedDataLength = data->EncryptedDataLength;
    return true;
}

bool TencentSDKWarperRecord::onAudioFrameDecrypt(const char *userId, TRTCCustomEncryptionData *data){
    //
    printf(" audio decrypt len:%d\n", data->EncryptedDataLength);
    for (int i = 0; i < data->EncryptedDataLength; i ++) {
        data->unEncryptedData[i] = ~data->EncryptedData[i];
    }
    data->unEncryptedDataLength = data->EncryptedDataLength;
    return true;
}

#pragma mark - ITRTCAudioRecvCallback
void TencentSDKWarperRecord::onRecvAudioFrame(const char *userId, TRTCAudioFrame *frame) {
    if (_need_audio == false) {
        printf("【StreamRec】E audio not enabled!\n");
        return;
    }
    
    std::shared_ptr<StreamRecorder> recorder = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto rec = _stream_recorders.find(userId);
        if (rec == _stream_recorders.end()) {
            recorder = std::make_shared<StreamRecorder>(_mode, _need_audio, _need_video, _recorder);
            // 控制混流行为的。
            recorder->setInfo(0xFFFFFF, 30);
            _stream_recorders[userId] = recorder;
        } else {
            recorder = rec->second;
        }
    }
    
    if (recorder) {
        recorder->addAudioFrame(userId, frame);
    }
    
}

#pragma mark - ITRTCVideoRecvCallback
void TencentSDKWarperRecord::onRecvVideoFrame(const char *userId, TRTCVideoStreamType streamType, TRTCVideoFrame *frame) {
    if (_need_video == false) {
        printf("【StreamRec】E video not enabled!\n");
        return;
    }
    std::shared_ptr<StreamRecorder> recorder = nullptr;
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        auto rec = _stream_recorders.find(userId);
        if (rec == _stream_recorders.end()) {
            recorder = std::make_shared<StreamRecorder>(_mode, _need_audio, _need_video, _recorder);
            recorder->setInfo(0xFFFFFF, 30);
            _stream_recorders[userId] = recorder;
        } else {
            recorder = rec->second;
        }
    }
    if (recorder && streamType == TRTCVideoStreamType::TRTCVideoStreamTypeBig ) { // 只录主流画面。
        recorder->addVideoFrame(userId, streamType, frame);
    }
}


#pragma mark - ITRTCMediaRecorderCallback
void TencentSDKWarperRecord::onFinished(const char *userId, int output, const char *filepath){
    // 单个文件录制完成通知，可以异步执行 转码或转封装的任务。
    printf("【StreamRec】file %s of user %s with type %d recorded!\n", filepath, userId, output);
}

