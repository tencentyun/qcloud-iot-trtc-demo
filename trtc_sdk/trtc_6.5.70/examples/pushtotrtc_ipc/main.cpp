#include <stdio.h>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <fstream>
#include <mutex>
#include <streambuf>
#include <sys/time.h>
#include <string.h>
#include "TRTCCloud.h"
#include "TRTCMediaStreamReader.h"

typedef struct Account {
    uint32_t    sdkappid;
    std::string userid;
    std::string usersig;
} AccountInfo;

AccountInfo g_account_info;

uint64_t getTimeStamp()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_sec * 1000 + time.tv_usec / 1000;
}

enum class EncrytionMode {
    kNone        = 0,
    kEncryptOnly = 1,
    kDecryptOnly = 2,
};

class LogCallback : public ITRTCLogCallback
{
   public:
    void onLog(const char *log, TRTCLogLevel level, const char *module);
};

class TencentSDKWarper : public ITRTCCloudCallback,
                         public ITRTCAudioRecvCallback,
                         public ITRTCVideoRecvCallback,
                         public ITRTCEncryptionCallback,
                         public ITRTCDecryptionCallback,
                         public IMediaStreamDataListener,
                         public IMediaPlayNotifyListener
{
   public:
    TencentSDKWarper(uint32_t sdkappid);
    virtual ~TencentSDKWarper();

    void start(TRTCParams &params, EncrytionMode mode);
    void stop();

    void setPushFile(std::string path);
    void sendSEIMsg(std::string data);
    void sendCmdMsg(std::string msg);

    void startPush();
    void stopPush();

   protected:
    void onError(TXLiteAVError errCode, const char *errMsg, void *arg) override;
    void onWarning(TXLiteAVWarning warningCode, const char *warningMsg, void *arg) override;
    void onEnterRoom(uint64_t elapsed) override;
    void onExitRoom(int reason) override;
    void onUserEnter(const char *userId) override;
    void onUserExit(const char *userId, int reason) override;
    void onFirstVideoFrame(const char *userId, uint32_t width, uint32_t height) override;
    void onFirstAudioFrame(const char *userId) override;
    void onUserVideoAvailable(const char *userId, bool available) override{};
    void onUserAudioAvailable(const char *userId, bool available) override{};
    void onConnectionLost() override{};
    void onTryToReconnect() override{};
    void onConnectionRecovery() override{};
    void onRecvSEIMsg(const char *userId, const unsigned char *data, int datalen) override;
    void onRecvCustomCmdMsg(const char *userId, int cmdId, int seq, const unsigned char *msg, int msglen) override;
    void onMissCustomCmdMsg(const char *userId, int cmdId, int errCode, int missed) override;

    void onRecvAudioFrame(const char *userId, TRTCAudioFrame *frame) override;
    void onRecvVideoFrame(const char *userId, TRTCVideoStreamType streamType, TRTCVideoFrame *frame) override;

    bool onAudioFrameEncrypt(TRTCCustomEncryptionData *data) override;
    bool onAudioFrameDecrypt(const char *userId, TRTCCustomEncryptionData *data) override;
    bool onVideoFrameEncrypt(TRTCVideoStreamType streamType, TRTCCustomEncryptionData *data) override;
    bool onVideoFrameDecrypt(const char *userId, TRTCVideoStreamType streamType,
                             TRTCCustomEncryptionData *data) override;

   private:
    void onMediaStreamVideoDecode(TRTCDecodeVideoFrame *frame) override;
    void onMediaStreamAudioDecode(TRTCDecodeAudioFrame *frame) override;
    void onMediaPlayerNotifyEvent(int event, void *param) override;

    ITRTCCloud *_tencentEngine;

    ITRTCMediaStreamReader *_streamreader;

    std::string _firstRemoteUserId;

    std::vector<TRTCAudioFrame> _audioCache;

    bool _bEnderRoom;

    std::string _url;

    std::mutex _mutexEngine;

    int m_curwidth = 0;

    int m_curheight = 0;

    int m_videofps = 0;

    uint32_t m_sdkappid;

    std::string getEventMsgByCode(int event);

    EncrytionMode m_encrytion_mode = EncrytionMode::kNone;
};

TencentSDKWarper::TencentSDKWarper(uint32_t sdkappid) : _firstRemoteUserId(""), _bEnderRoom(false), m_sdkappid(sdkappid)
{
    _tencentEngine = createInstance(m_sdkappid);
    _tencentEngine->setCallback(this);

    _streamreader = createMSRInstance();
    _streamreader->setDataListener(this);
    _streamreader->setNotify(this);
    TRTCStreamParam param;
    param.isLoop = false;
    _streamreader->setStreamParam(&param);
}

TencentSDKWarper::~TencentSDKWarper()
{
    puts("~MyRecorder");
    {
        std::lock_guard<std::mutex> lock(_mutexEngine);
        destroyInstance(_tencentEngine);
        _tencentEngine = NULL;
    }
    destroyMSRInstance(_streamreader);
    _streamreader = NULL;
}

std::string TencentSDKWarper::getEventMsgByCode(int event)
{
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

void TencentSDKWarper::start(TRTCParams &params, EncrytionMode mode)
{
    m_encrytion_mode = mode;
    if (_tencentEngine) {
        _tencentEngine->enterRoom(params, TRTCAppScene::TRTCAppSceneVideoCall);
    }
}

void TencentSDKWarper::stop()
{
    if (_tencentEngine) {
        _tencentEngine->exitRoom();
    }
}

void TencentSDKWarper::sendSEIMsg(std::string data)
{
    if (_tencentEngine) {
        if (_tencentEngine->sendSEIMsg((const unsigned char *)data.c_str(), data.length(), 1)) {
            printf("send sei msg  success!!!\n");
        } else {
            printf("send sei msg failed!!!\n");
        }
    }
}

void TencentSDKWarper::sendCmdMsg(std::string msg)
{
    if (_tencentEngine) {
        if (_tencentEngine->sendCustomCmdMsg(1, (const unsigned char *)msg.c_str(), msg.length(), true, true)) {
            printf("send custom msg sucess!\n");
        } else {
            printf("send custom msg failed!\n");
        }
    }
}

void TencentSDKWarper::setPushFile(std::string path)
{
    _url = path;
}

void TencentSDKWarper::startPush()
{
    if (_streamreader) {
        _streamreader->start(_url.c_str());
    }
}

void TencentSDKWarper::stopPush()
{
    if (_streamreader) {
        _streamreader->stop();
    }
}

#pragma mark - ITRTCCloudCallback
void TencentSDKWarper::onRecvSEIMsg(const char *userId, const unsigned char *data, int datalen)
{
    printf("onRecvSEIMsg  userid %s  data %s \n", userId, data);
}

void TencentSDKWarper::onError(TXLiteAVError errCode, const char *errMsg, void *arg) {}

void TencentSDKWarper::onWarning(TXLiteAVWarning warningCode, const char *warningMsg, void *arg) {}

void TencentSDKWarper::onEnterRoom(uint64_t elapsed)
{
    if (m_encrytion_mode == EncrytionMode::kEncryptOnly) {
        if (_tencentEngine) {
            _tencentEngine->setEncryptionCallback(this);
        }
    } else if (m_encrytion_mode == EncrytionMode::kDecryptOnly) {
        if (_tencentEngine) {
            _tencentEngine->setDecryptionCallback(this);
        }
    }
    _bEnderRoom = true;
    startPush();
}

void TencentSDKWarper::onExitRoom(int reason)
{
    _bEnderRoom = false;
    stopPush();
}

void TencentSDKWarper::onUserEnter(const char *userId)
{
    std::lock_guard<std::mutex> lock(_mutexEngine);
    if (_tencentEngine) {
        _tencentEngine->setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_AAC, this);
        _tencentEngine->setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_YUVI420, this);
    }
}

void TencentSDKWarper::onUserExit(const char *userId, int reason)
{
    std::lock_guard<std::mutex> lock(_mutexEngine);
    if (_tencentEngine) {
        _tencentEngine->setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_Unknown, nullptr);
        _tencentEngine->setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_Unknown, nullptr);
    }
}

void TencentSDKWarper::onFirstAudioFrame(const char *userId) {}

void TencentSDKWarper::onFirstVideoFrame(const char *userId, uint32_t width, uint32_t height) {}

void TencentSDKWarper::onRecvCustomCmdMsg(const char *userId, int cmdId, int seq, const unsigned char *msg, int msglen)
{
    printf("userid %s cmd %d seq %d msg %s meslen %d\n", userId, cmdId, seq, msg, msglen);
}

void TencentSDKWarper::onMissCustomCmdMsg(const char *userId, int cmdId, int errCode, int missed)
{
    printf("userid %s cmd %d code %d missed %d\n", userId, cmdId, errCode, missed);
}

void LogCallback::onLog(const char *log, TRTCLogLevel level, const char *module)
{
    // printf("[%s][l:%d][%s]\n", module, level, log);
}

#pragma mark - ITRTCAudioRecvCallback
void TencentSDKWarper::onRecvAudioFrame(const char *userId, TRTCAudioFrame *frame)
{
    //    printf("onRecvAudioFrame:  userid %s timestamp %llu\n", userId, frame->timestamp);
}

#pragma mark - ITRTCVideoRecvCallback
void TencentSDKWarper::onRecvVideoFrame(const char *userId, TRTCVideoStreamType streamType, TRTCVideoFrame *frame)
{
    printf("onRecvVideoFrame:  userid %s timestamp %llu type %d\n", userId, frame->timestamp, streamType);
}

#pragma mark - ITRTCEncryptionCallback
bool TencentSDKWarper::onVideoFrameEncrypt(TRTCVideoStreamType streamType, TRTCCustomEncryptionData *data)
{
    // 必须是同步进行加密。
    // data 的内存由SDK负责申请和释放的。
    // 针对主视频流进行加密，加密算法 按位取反。
    printf(" video custom encryption len:%d\n", data->unEncryptedDataLength);
    if (streamType == TRTCVideoStreamType::TRTCVideoStreamTypeBig) {
        for (int i = 0; i < data->unEncryptedDataLength; i++) {
            data->EncryptedData[i] = ~data->unEncryptedData[i];
        }
    }
    // 很重要，加密后的数据的长度。
    data->EncryptedDataLength = data->unEncryptedDataLength;
    return true;
}

bool TencentSDKWarper::onAudioFrameEncrypt(TRTCCustomEncryptionData *data)
{
    // 必须是同步进行加密。
    printf(" audio custom encryption len:%d\n", data->unEncryptedDataLength);
    for (int i = 0; i < data->unEncryptedDataLength; i++) {
        data->EncryptedData[i] = ~data->unEncryptedData[i];
    }
    data->EncryptedDataLength = data->unEncryptedDataLength;
    return true;
}

#pragma mark - ITRTCDecryptionCallback
bool TencentSDKWarper::onVideoFrameDecrypt(const char *userId, TRTCVideoStreamType streamType,
                                           TRTCCustomEncryptionData *data)
{
    // 必须是同步进行解密。
    // data 的内存由SDK负责申请和释放的。
    // 针对主视频流进行解密，解密算法 按位取反。
    printf(" video custom decryption len:%d\n", data->EncryptedDataLength);
    if (streamType == TRTCVideoStreamType::TRTCVideoStreamTypeBig) {
        for (int i = 0; i < data->EncryptedDataLength; i++) {
            data->unEncryptedData[i] = ~data->EncryptedData[i];
        }
    }
    // 很重要，加密后的数据的长度。
    data->unEncryptedDataLength = data->EncryptedDataLength;
    return true;
}

bool TencentSDKWarper::onAudioFrameDecrypt(const char *userId, TRTCCustomEncryptionData *data)
{
    //
    printf(" audio custom decryption len:%d\n", data->EncryptedDataLength);
    for (int i = 0; i < data->EncryptedDataLength; i++) {
        data->unEncryptedData[i] = ~data->EncryptedData[i];
    }
    data->unEncryptedDataLength = data->EncryptedDataLength;
    return true;
}

#pragma mark - IMediaStreamDataListener
void TencentSDKWarper::onMediaStreamAudioDecode(TRTCDecodeAudioFrame *frame)
{
    if (_bEnderRoom == true) {
        //        printf("[%llu] send audio timestamp %llu length %u samplerate %d channel %d \n",getTimeStamp(),
        //        frame->timestamp, frame->length, frame->sample, frame->channels);
        TRTCAudioFrame audioframe;
        audioframe.data        = (uint8_t *)frame->data;
        audioframe.length      = frame->length;
        audioframe.sampleRate  = frame->sample;
        audioframe.channel     = frame->channels;
        audioframe.timestamp   = frame->timestamp;
        audioframe.audioFormat = TRTCAudioFrameFormat::TRTCAudioFrameFormat_PCM;
        {
            std::lock_guard<std::mutex> lock(_mutexEngine);
            if (_tencentEngine) {
                _tencentEngine->sendCustomAudioData(&audioframe);
            }
        }
    } else {
        printf("do not send audio timestamp %llu length %u samplerate %d channel %d \n", frame->timestamp,
               frame->length, frame->sample, frame->channels);
    }
}

void TencentSDKWarper::onMediaStreamVideoDecode(TRTCDecodeVideoFrame *frame)
{
    if (_bEnderRoom == true) {
        //        printf("[%llu] send video timestamp %llu length %u width %u height %u\n", getTimeStamp(),
        //        frame->timestamp, frame->length, frame->width, frame->height);
        TRTCVideoFrame videoframe;
        videoframe.data        = (uint8_t *)frame->data;
        videoframe.bufferType  = TRTCVideoBufferType::TRTCVideoBufferType_Buffer;
        videoframe.length      = frame->length;
        videoframe.rotation    = TRTCVideoRotation::TRTCVideoRotation0;
        videoframe.width       = frame->width;
        videoframe.height      = frame->height;
        videoframe.timestamp   = frame->timestamp;
        videoframe.videoFormat = TRTCVideoFrameFormat::TRTCVideoFrameFormat_YUVI420;

        if (m_curwidth != frame->width || m_curheight != frame->height) {
            // 分辨率变化了需要设置编码参数。
            TRTCVideoEncParam param;
            param.videoFps = m_videofps;
            // 设置经验公式，具体也可以参考分辨率 码率 建议值 详见TRTCCloudDef.h 文件。
            param.videoBitrate = frame->width * frame->height * 1.7 / 1000;
            std::lock_guard<std::mutex> lock(_mutexEngine);
            if (_tencentEngine) {
                _tencentEngine->setVideoEncoderParam(param);
                printf("\nwidth %d height %d\n", frame->width, frame->height);
            }

            m_curheight = frame->height;
            m_curwidth  = frame->width;
        }

        {
            std::lock_guard<std::mutex> lock(_mutexEngine);
            if (_tencentEngine) {
                _tencentEngine->sendCustomVideoData(&videoframe);
            }
        }
    } else {
        printf("do not send video timestamp %llu length %u width %u height %u\n", frame->timestamp, frame->length,
               frame->width, frame->height);
    }
}

void TencentSDKWarper::onMediaPlayerNotifyEvent(int event, void *param)
{
    printf("\n --------------  event %s(%d) --------------\n", getEventMsgByCode(event).c_str(), event);

    if (event == EVENT_MEDIAPLAY_PLAY_SUCCESS) {
        std::lock_guard<std::mutex> lock(_mutexEngine);
        if (_streamreader) {
            TRTCStreamInfo info;
            _streamreader->getFileInfo(&info);
            printf("--- duration %lld  sduration %s fps %d video width %d video height %d --\n", info.duration,
                   info.str_duration, info.videoFps, info.videoWidth, info.videoheight);

            TRTCVideoEncParam param;
            m_videofps     = info.videoFps;
            m_curheight    = info.videoheight;
            m_curwidth     = info.videoWidth;
            param.videoFps = info.videoFps;
            // 码率经验公式，供参考。
            param.videoBitrate = info.videoWidth * info.videoheight * 1.7 / 1000;
            if (_tencentEngine) {
                _tencentEngine->setVideoEncoderParam(param);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 从文件读取TRTC账号信息
// 文件格式如下:注意 = 号左右有一个空格
//       sdkappid = 1240000001
//       userid = test1
//       usersig = XXXXXXXXXXXXXXXXXXXXXXXXXXXXX

bool getAccountFromFile(const char *filepath, uint32_t &sdkappid, std::string &userid, std::string &usersig)
{
    std::ifstream fin(filepath);
    if (!fin.is_open()) {
        printf(
            "open file %s failed! make sure file exist!\n it's content like:\n sdkappid = 14000000001\n userid = "
            "test_id_1\n usersig = XXXXXXXXXXXXXXXX\n",
            filepath);
        return false;
    }
    char buffer[4096];
    int  buffercap = sizeof(buffer);
    // 字符串分割。
    auto split = [](std::string &str, std::string &pattern) -> std::vector<std::string> {
        std::vector<std::string> ret;
        if (str.empty()) {
            return ret;
        }

        std::string temp = str + pattern;  // 方便后面截取

        size_t pos  = temp.find(pattern);
        size_t size = temp.size();
        while (pos != std::string::npos) {
            std::string item = temp.substr(0, pos);
            ret.push_back(item);
            temp = temp.substr(pos + pattern.size(), size);
            pos  = temp.find(pattern);
        }

        return ret;
    };

    // 去掉所有空格。
    auto trim = [](std::string &str) -> std::string {
        int index = 0;
        if (!str.empty()) {
            while ((index = str.find(' ', index)) != std::string::npos) {
                str.erase(index, 1);
            }
        }
        return str;
    };

    while (fin.getline(buffer, buffercap)) {
        std::string line;
        line.append(buffer, strlen(buffer));
        std::string pattern;
        pattern.append(" = ");
        std::vector<std::string> items = split(line, pattern);
        if (items.size() == 2) {
            std::string key = items[0];
            key             = trim(key);

            std::string value = items[1];
            value             = trim(value);

            if (key.compare("sdkappid") == 0) {
                sdkappid = std::stoi(value);
            } else if (key.compare("userid") == 0) {
                userid = value;
            } else if (key.compare("usersig") == 0) {
                usersig = value;
            }
        }
    }
    fin.clear();
    fin.close();
    return true;
}

//#define STRING_ROOM // 房间号是一个字符串，请打开这个宏。否则注释掉。
void testSingleRoom(const std::string &roomName, std::string filepath, EncrytionMode mode)
{
    printf("start testSingleRoom: roomName[%s]\n", roomName.c_str());
    // step 1 进房参数
    TRTCParams params;
    params.sdkAppId     = g_account_info.sdkappid;  // 进房参数设置 sdkappid
    std::string userid  = g_account_info.userid;    // 进房参数设置 userid
    std::string usersig = g_account_info.usersig;   // 进房参数设置 usersig
    params.userId       = userid;
    params.userSig      = usersig;
#ifdef STRING_ROOM
    std::string bussinfo = "{\"strGroupId\": \"" + roomName + "\"}";
    params.roomId        = -1;
    params.businessInfo  = bussinfo;
#else
    params.roomId       = std::stoi(roomName);
    params.businessInfo = roomName;
#endif

    // step 2 开始进房
    auto engine = std::make_shared<TencentSDKWarper>(g_account_info.sdkappid);
    engine->setPushFile(filepath.c_str());
    engine->start(params, mode);

    std::this_thread::sleep_for(std::chrono::seconds(120));

    // step 3 退房
    engine->stop();
    printf("testSingleRoom finished: roomName[%s]\n", roomName.c_str());
}

int main(int argc, char *argv[])
{
    std::string roomid = "66666";
    if (argc >= 2) {
        roomid = argv[1];
    }

    //    std::string filepath ="./resource/ruguo-640x360.mp4";
    std::string filepath = "../resource/ruguo-640x360.mp4";
    if (argc >= 3) {
        filepath = argv[2];
    }

    // 配置log路径
    std::shared_ptr<LogCallback> log;
    log.reset(new LogCallback());
    setLogDirPath("/tmp/log");
    setConsoleEnabled(true);

    /// 用跨进程方案必须调用该方法。
    //    std::string core
    //    ="/Users/arvinwu/Documents/gitcode/linuxSDK/project/linux/sdk/push_ipc_core/project/Debug/TrtcCoreService";
    std::string core = "../../bin/TrtcCoreService";

    /// rootpath 过长会导致进程间通信建立失败，推荐设置为/tmp 或者 /data 目录。
    std::string rootpath = "/tmp";
    setIPCParam((char *)core.c_str(), (char *)rootpath.c_str());

    // 读取账号文件 获取 sdkappid userid 和 usersig（安全票据），若需要测试用的账号，可以微信联系我们。
    if (getAccountFromFile("account.txt", g_account_info.sdkappid, g_account_info.userid, g_account_info.usersig) ==
        false) {
        return 0;
    }

    // 进房操作，房间号需要为数字。
    int roomnum = std::stoi(roomid);

    // 默认同时推2个房间。这里简化了用了同一个账号，对接的时候，请注意，用不同的账号。
    constexpr int num = 2;

    std::thread threads[num] = {std::thread(testSingleRoom, std::to_string(roomnum), filepath, EncrytionMode::kNone),
                                std::thread(testSingleRoom, std::to_string(++roomnum), filepath, EncrytionMode::kNone)};

    for (int i = 0; i < num; i++) {
        if (threads[i].joinable()) {
            threads[i].join();
        }
    }

    return 0;
}
