#include "TencentSDKWapperMixRecord.h"

#pragma mark - LayoutMgr impl
LayoutMgr::LayoutMgr(int s_width, int s_height, int s_count, int t_width, int t_height, int v_margin, int h_margin)
: student_width(s_width)
, student_height(s_height)
, student_count(s_count)
, teacher_width(t_width)
, teacher_height(t_height)
, v_margin(v_margin)
, h_margin(h_margin)
{
    canvas_width = 2*h_margin + s_count*(student_width+h_margin) + teacher_width;
    int height = teacher_height>student_height?teacher_height:student_height;
    canvas_height = 2*v_margin + height;
}

LayoutMgr::~LayoutMgr(){
    students.clear();
}

void LayoutMgr::addStudent(std::string userid){
    delStudent(userid);
    if (students.size() >= student_count) {
        return;
    }
    students.push_back(userid);
}

void LayoutMgr::delStudent(std::string userid){
    auto it = find(students.begin(), students.end(), userid);
    if (it != students.end()) {
        students.erase(it);
    }
}

void LayoutMgr::setTeacher(std::string userid){
    teacher = userid;
}

void LayoutMgr::getRegions(std::map<std::string, Region> &regions, int &canvas_width, int &canvas_height){
    canvas_width = this->canvas_width;
    canvas_height = this->canvas_height;
    // 第一个是老师。
    Region r;
    r.zOrder = 1;
    r.width = teacher_width;
    r.height = teacher_height;
    r.offset_x = h_margin;
    r.offset_y = v_margin;
    r.bgcolor = 0x008000;
    r.option = REGION_OPTION_FILLMODE_FIT;
    
    if (!teacher.empty()) {
        regions[teacher] = r;
    }
    
    // 学生布局。
    int i=0;
    for (auto it : students) {
        r.zOrder = 2+i;
        r.width = student_width;
        r.height = student_height;
        r.offset_x = teacher_width + 2*h_margin + (h_margin+student_width)*i;
        r.offset_y = v_margin;
        r.bgcolor = 0x000080;
        r.option = REGION_OPTION_FILLMODE_FIT;
        regions[it] = r;
        i++;
    }
}

#pragma mark - TencentSDKWarperMixRecord impl
namespace  {
    constexpr int REASON_FAIL = 2;
    constexpr int REASON_OK = 1;
    constexpr int REASON_UNKNOWN = 0;
};

TencentSDKWarperMixRecord::TencentSDKWarperMixRecord(uint32_t sdkappid)
: _mixname("mixer")
, _sdkappid(sdkappid)
, _anchor_count(0)
, _reason(REASON_UNKNOWN)
, _started(false)
, _mix_audio(false)
, _mix_video(false)
{
    // 初始化房间组件
    _room = createInstance(_sdkappid);
    _room->setCallback(this);
    
    // 初始化混流组件
    _mixer = createMediaMixer();
    _mixer->setCallback(this);
    
    // 初始化录制组件
    _recorder = createMediaRecorder();
    _recorder->setCallback(this);
    
    // 初始化布局管理器, 老师 100*100，3个学生 100*100，水平边距10，垂直边距10。 单位像素。
    _layout.reset(new LayoutMgr(100,100,3,100,100,10,10));
}

TencentSDKWarperMixRecord::~TencentSDKWarperMixRecord() {
    _room->setCallback(nullptr);
    destroyInstance(_room);
    _room = NULL;
    
    _mixer->setCallback(nullptr);
    destroyMediaMixer(_mixer);
    _mixer = NULL;

    _recorder->setCallback(nullptr);
    destroyMediaRecorder(_recorder);
    _recorder = NULL;
    
    _layout.reset();
}

void TencentSDKWarperMixRecord::start(TRTCParams &params, const char* record_roomid, const char* streamname, int output, const char *dir) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_started == true) {
        return;
    }
    _started = true;
    
    _mixname = streamname;
    
    if (_room) {
        _room->enterRoom(params, TRTCAppScene::TRTCAppSceneVideoCall);
    }
    
    if (output & 1 || output & 4) {
        _mix_audio = true;
    }
    
    if (output & 2 || output & 4) {
        _mix_video = true;
    }

    
    if (_recorder) {
        _recorder->setParam(_sdkappid, record_roomid, (char*)params.userId.buffer, output, dir);
    }
    
    if (_mixer){
        std::map<std::string, Region> regions;
        int canvas_width = 0;
        int canvas_height = 0;
        _layout->getRegions(regions, canvas_width, canvas_height);
        _mixer->setCanvas(20, canvas_width, canvas_height, 0xFFFFFF);
        _mixer->start(_mix_audio, _mix_video);// 混音 混图。
    }
}

void TencentSDKWarperMixRecord::stop() {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_started == false) {
        return;
    }
    _started = false;
    
    if (_room) {
        _room->exitRoom();
    }

    if (_mixer){
        _mixer->stop();
    }
    
    if (_recorder) {
        _recorder->stopAll();
    }
}

bool TencentSDKWarperMixRecord::isRunning(int &raeson) {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    if (_reason == REASON_FAIL) {
        return false;
    }
    
    if (_reason == REASON_OK && _anchor_count == 0) {
        return false;
    }
    
    return true;
}

void TencentSDKWarperMixRecord::updateLayout(){
    if (_mixer) {
        std::map<std::string, Region> regions;
        int canvas_width = 0;
        int canvas_height = 0;
        _layout->getRegions(regions, canvas_width, canvas_height);
        
        _mixer->clearRegions();
        for (auto it : regions) {
            _mixer->setRegion((char*)it.first.c_str(), &it.second);
        }
        _mixer->applyRegions();
    }
}

#pragma mark - ITRTCCloudCallback
void TencentSDKWarperMixRecord::onRecvSEIMsg(const char *userId, const unsigned char *data, int datalen){
}

void TencentSDKWarperMixRecord::onError(TXLiteAVError errCode, const char *errMsg, void *arg ) {
    // ITRTCCloud 组件内部报错，说明功能不可用了。建议打印错误信息，然后退出房间。
     std::lock_guard<std::recursive_mutex> lock(_mutex);
    _reason = REASON_FAIL;
}

void TencentSDKWarperMixRecord::onWarning(TXLiteAVWarning warningCode, const char *warningMsg, void *arg) {
}

void TencentSDKWarperMixRecord::onEnterRoom(uint64_t elapsed) {
}

void TencentSDKWarperMixRecord::onExitRoom(int reason) {

}

void TencentSDKWarperMixRecord::onUserEnter(const char *userId) {
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        ++_anchor_count;
        _reason = REASON_OK;
    }
    
    // 如果关心 userId 这个用户的流，需要主动设置数据回调，不关心的话就不用设置，数据就不会回调上来
    // example:
    // if (isInBlackList(userId)){
    //     return;
    // }
    
    if (_layout->getTeacher().empty()) {
        _layout->setTeacher(userId);
        updateLayout();
    }else{
        if (_layout->getTeacher() != userId) {
            _layout->addStudent(userId);
            updateLayout();
        }else{// 应该执行不到。
            _layout->setTeacher(userId);
            updateLayout();
        }
    }
    
    if (_room) {
        if (_mix_audio) {
            _room->setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_PCM, this);
        }
        if (_mix_video) {
            _room->setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_YUVI420, this);
        }
    }
}

void TencentSDKWarperMixRecord::onUserExit(const char *userId, int reason) {
    {
        std::lock_guard<std::recursive_mutex> lock(_mutex);
        --_anchor_count;
    }
    
    if (_layout->getTeacher() != userId) {
        _layout->delStudent(userId);
        updateLayout();
    }else{
        _layout->setTeacher("");
        updateLayout();
    }
    
    // 不关心 userid 的 音视频数据了，主动设置一下是个好习惯。
    if (_room) {
        if (_mix_audio) {
            _room->setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_Unknown, nullptr);
        }
        
        if (_mix_video) {
            _room->setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_Unknown, nullptr);
        }
    }
}

void TencentSDKWarperMixRecord::onFirstAudioFrame(const char *userId) {
}

void TencentSDKWarperMixRecord::onFirstVideoFrame(const char *userId, uint32_t width, uint32_t height) {
}

#pragma mark - ITRTCAudioRecvCallback
void TencentSDKWarperMixRecord::onRecvAudioFrame(const char *userId, TRTCAudioFrame *frame) {
    if (_mixer) {
        _mixer->addAudioFrame(userId, frame);
    }
}

#pragma mark - ITRTCVideoRecvCallback
void TencentSDKWarperMixRecord::onRecvVideoFrame(const char *userId, TRTCVideoStreamType streamType, TRTCVideoFrame *frame) {
    if (_mixer) {
        _mixer->addVideoFrame(userId, frame);
    }
}

#pragma mark - ITRTCMediaMixerCallback
void TencentSDKWarperMixRecord::onError(int errcode, char *errmsg){
    
}

void TencentSDKWarperMixRecord::onMixedAudioFrame(TRTCAudioFrame *frame){
    if (_recorder) {
        _recorder->addAudioFrame("mixer", frame);
    }
}

void TencentSDKWarperMixRecord::onMixedVideoFrame(TRTCVideoFrame *frame){
    if (_recorder) {
        _recorder->addVideoFrame("mixer", TRTCVideoStreamType::TRTCVideoStreamTypeBig, frame);
    }
}

#pragma mark - ITRTCMediaRecorderCallback
void TencentSDKWarperMixRecord::onFinished(const char *userId, int output, const char *filepath){
    // 单个文件录制完成通知，可以异步执行 转码或转封装的任务。
}
