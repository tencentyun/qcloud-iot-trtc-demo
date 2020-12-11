/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */

#include <atomic>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <stdio.h>
#include <streambuf>
#include <string.h>
#include <sys/time.h>
#include <thread>
#include <vector>

#include "TRTCCloud.h"
#include "TRTCMediaStreamReader.h"

uint64_t getTimeStamp() {
  struct timeval time;
  gettimeofday(&time, NULL);
  return time.tv_sec * 1000 + time.tv_usec / 1000;
}

enum class EncrytionMode {
  kNone = 0,
  kEncryptOnly = 1,
  kDecryptOnly = 2,
};

class LogCallback : public ITRTCLogCallback {
public:
  void onLog(const char *log, TRTCLogLevel level, const char *module);
};

class TencentSDKWarper : public ITRTCCloudCallback,
                         public ITRTCAudioRecvCallback,
                         public ITRTCVideoRecvCallback,
                         public ITRTCEncryptionCallback,
                         public ITRTCDecryptionCallback,
                         public IMediaStreamDataListener,
                         public IMediaPlayNotifyListener {
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

  bool getUsrRoomStat() { return _bUsrEnterRoom; }
  void setAVFlag(int av_flag) { m_av_flag = av_flag; }

protected:
  void onError(TXLiteAVError errCode, const char *errMsg, void *arg) override;
  void onWarning(TXLiteAVWarning warningCode, const char *warningMsg,
                 void *arg) override;
  void onEnterRoom(uint64_t elapsed) override;
  void onExitRoom(int reason) override;
  void onUserEnter(const char *userId) override;
  void onUserExit(const char *userId, int reason) override;
  void onFirstVideoFrame(const char *userId, uint32_t width,
                         uint32_t height) override;
  void onFirstAudioFrame(const char *userId) override;
  void onUserVideoAvailable(const char *userId, bool available) override{};
  void onUserAudioAvailable(const char *userId, bool available) override{};
  void onConnectionLost() override{};
  void onTryToReconnect() override{};
  void onConnectionRecovery() override{};
  void onRecvSEIMsg(const char *userId, const unsigned char *data,
                    int datalen) override;
  void onRecvCustomCmdMsg(const char *userId, int cmdId, int seq,
                          const unsigned char *msg, int msglen) override;
  void onMissCustomCmdMsg(const char *userId, int cmdId, int errCode,
                          int missed) override;

  void onRecvAudioFrame(const char *userId, TRTCAudioFrame *frame) override;
  void onRecvVideoFrame(const char *userId, TRTCVideoStreamType streamType,
                        TRTCVideoFrame *frame) override;

  bool onAudioFrameEncrypt(TRTCCustomEncryptionData *data) override;
  bool onAudioFrameDecrypt(const char *userId,
                           TRTCCustomEncryptionData *data) override;
  bool onVideoFrameEncrypt(TRTCVideoStreamType streamType,
                           TRTCCustomEncryptionData *data) override;
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

  bool _bUsrEnterRoom;

  std::string _url;

  std::mutex _mutexEngine;

  int m_curwidth = 0;

  int m_curheight = 0;

  int m_videofps = 0;

  int m_av_flag = 0;

  uint32_t m_sdkappid;

  FILE *m_audio_file;

  std::string getEventMsgByCode(int event);

  EncrytionMode m_encrytion_mode = EncrytionMode::kNone;
};

TencentSDKWarper::TencentSDKWarper(uint32_t sdkappid)
    : _firstRemoteUserId(""), _bEnderRoom(false), m_sdkappid(sdkappid) {
  _tencentEngine = createInstance(m_sdkappid);
  _tencentEngine->setCallback(this);

  _bUsrEnterRoom = false;
  m_av_flag = 0;
  _streamreader = createMSRInstance();
  _streamreader->setDataListener(this);
  _streamreader->setNotify(this);
  TRTCStreamParam param;
  param.isLoop = false;
  _streamreader->setStreamParam(&param);
  m_audio_file = NULL;
}

TencentSDKWarper::~TencentSDKWarper() {
  puts("~MyRecorder");
  {
    std::lock_guard<std::mutex> lock(_mutexEngine);
    destroyInstance(_tencentEngine);
    _tencentEngine = NULL;
  }
  destroyMSRInstance(_streamreader);
  _streamreader = NULL;
  if (m_audio_file) {
    fclose(m_audio_file);
    m_audio_file = NULL;
  }
}

std::string TencentSDKWarper::getEventMsgByCode(int event) {
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

void TencentSDKWarper::start(TRTCParams &params, EncrytionMode mode) {
  m_encrytion_mode = mode;
  if (_tencentEngine) {
    _tencentEngine->enterRoom(params, TRTCAppScene::TRTCAppSceneVideoCall);
  }
}

void TencentSDKWarper::stop() {
  if (_tencentEngine) {
    _tencentEngine->exitRoom();
  }
}

void TencentSDKWarper::sendSEIMsg(std::string data) {
  if (_tencentEngine) {
    if (_tencentEngine->sendSEIMsg((const unsigned char *)data.c_str(),
                                   data.length(), 1)) {
      printf("send sei msg  success!!!\n");
    } else {
      printf("send sei msg failed!!!\n");
    }
  }
}

void TencentSDKWarper::sendCmdMsg(std::string msg) {
  if (_tencentEngine) {
    if (_tencentEngine->sendCustomCmdMsg(1, (const unsigned char *)msg.c_str(),
                                         msg.length(), true, true)) {
      printf("send custom msg sucess!\n");
    } else {
      printf("send custom msg failed!\n");
    }
  }
}

void TencentSDKWarper::setPushFile(std::string path) { _url = path; }

void TencentSDKWarper::startPush() {
  if (_streamreader) {
    _streamreader->start(_url.c_str());
  }
}

void TencentSDKWarper::stopPush() {
  if (_streamreader) {
    _streamreader->stop();
  }
}

#pragma mark - ITRTCCloudCallback
void TencentSDKWarper::onRecvSEIMsg(const char *userId,
                                    const unsigned char *data, int datalen) {
  printf("onRecvSEIMsg  userid %s  data %s \n", userId, data);
}

void TencentSDKWarper::onError(TXLiteAVError errCode, const char *errMsg,
                               void *arg) {}

void TencentSDKWarper::onWarning(TXLiteAVWarning warningCode,
                                 const char *warningMsg, void *arg) {}

void TencentSDKWarper::onEnterRoom(uint64_t elapsed) {
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

  m_audio_file = fopen("./audio_file.pcm", "wb");
  if (!m_audio_file) {
    printf("open file failed!\n");
  } else {
    // printf("open file %s successful!\n", "./audio_file.pcm");
  }

  startPush();
}

void TencentSDKWarper::onExitRoom(int reason) {
  _bEnderRoom = false;
  stopPush();
  if (m_audio_file) {
    fflush(m_audio_file);
    fclose(m_audio_file);
    m_audio_file = NULL;
  }
}

void TencentSDKWarper::onUserEnter(const char *userId) {
  std::lock_guard<std::mutex> lock(_mutexEngine);
  if (_tencentEngine) {
    _tencentEngine->setRemoteAudioRecvCallback(
        userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_PCM, this);
    _tencentEngine->setRemoteVideoRecvCallback(
        userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_YUVI420, this);
  }
  _bUsrEnterRoom = true;
}

void TencentSDKWarper::onUserExit(const char *userId, int reason) {
  std::lock_guard<std::mutex> lock(_mutexEngine);
  if (_tencentEngine) {
    _tencentEngine->setRemoteAudioRecvCallback(
        userId, TRTCAudioFrameFormat::TRTCAudioFrameFormat_Unknown, nullptr);
    _tencentEngine->setRemoteVideoRecvCallback(
        userId, TRTCVideoFrameFormat::TRTCVideoFrameFormat_Unknown, nullptr);
  }
  _bUsrEnterRoom = false;
}

void TencentSDKWarper::onFirstAudioFrame(const char *userId) {}

void TencentSDKWarper::onFirstVideoFrame(const char *userId, uint32_t width,
                                         uint32_t height) {}

void TencentSDKWarper::onRecvCustomCmdMsg(const char *userId, int cmdId,
                                          int seq, const unsigned char *msg,
                                          int msglen) {
  printf("userid %s cmd %d seq %d msg %s meslen %d\n", userId, cmdId, seq, msg,
         msglen);
}

void TencentSDKWarper::onMissCustomCmdMsg(const char *userId, int cmdId,
                                          int errCode, int missed) {
  printf("userid %s cmd %d code %d missed %d\n", userId, cmdId, errCode,
         missed);
}

void LogCallback::onLog(const char *log, TRTCLogLevel level,
                        const char *module) {
  // printf("[%s][l:%d][%s]\n", module, level, log);
}

#pragma mark - ITRTCAudioRecvCallback
void TencentSDKWarper::onRecvAudioFrame(const char *userId,
                                        TRTCAudioFrame *frame) {
  // printf("onRecvAudioFrame:userid %s timestamp %lu, type:%d, data:%p,
  // length:%d!\n", userId, frame->timestamp,
  //       frame->audioFormat, frame->data, frame->length);

  if (m_audio_file) {
    fwrite(frame->data, 1, frame->length, m_audio_file);
    fflush(m_audio_file);
  }
}

#pragma mark - ITRTCVideoRecvCallback
void TencentSDKWarper::onRecvVideoFrame(const char *userId,
                                        TRTCVideoStreamType streamType,
                                        TRTCVideoFrame *frame) {
  // printf("onRecvVideoFrame:userid %s timestamp %lu type %d, w:%d, h:%d,
  // data:%p, length:%d, type:%d\n", userId,
  //       frame->timestamp, streamType, frame->width, frame->height,
  //       frame->data, frame->length, frame->videoFormat);
  const char *yuv_file = "./yuv_tmp.yuv";
  FILE *video_stream = fopen(yuv_file, "wbx");
  if (video_stream) {
    fwrite(frame->data, 1, frame->length, video_stream);
    fflush(video_stream);
    fclose(video_stream);
  }

  char command[128] = {0};
  sprintf(command,
          "mplayer -vo xv -demuxer rawvideo -rawvideo w=%d:h=%d:format=yv12 %s",
          frame->width, frame->height, yuv_file);
  // printf("onRecvVideoFrame:%s\n", command);
  // system(command);
}

#pragma mark - ITRTCEncryptionCallback
bool TencentSDKWarper::onVideoFrameEncrypt(TRTCVideoStreamType streamType,
                                           TRTCCustomEncryptionData *data) {
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

bool TencentSDKWarper::onAudioFrameEncrypt(TRTCCustomEncryptionData *data) {
  // 必须是同步进行加密。
  printf(" audio custom encryption len:%d\n", data->unEncryptedDataLength);
  for (int i = 0; i < data->unEncryptedDataLength; i++) {
    data->EncryptedData[i] = ~data->unEncryptedData[i];
  }
  data->EncryptedDataLength = data->unEncryptedDataLength;
  return true;
}

#pragma mark - ITRTCDecryptionCallback
bool TencentSDKWarper::onVideoFrameDecrypt(const char *userId,
                                           TRTCVideoStreamType streamType,
                                           TRTCCustomEncryptionData *data) {
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

bool TencentSDKWarper::onAudioFrameDecrypt(const char *userId,
                                           TRTCCustomEncryptionData *data) {
  //
  printf(" audio custom decryption len:%d\n", data->EncryptedDataLength);
  for (int i = 0; i < data->EncryptedDataLength; i++) {
    data->unEncryptedData[i] = ~data->EncryptedData[i];
  }
  data->unEncryptedDataLength = data->EncryptedDataLength;
  return true;
}

#pragma mark - IMediaStreamDataListener
void TencentSDKWarper::onMediaStreamAudioDecode(TRTCDecodeAudioFrame *frame) {
  if (_bEnderRoom == true) {
    // printf("[%lu] send audio timestamp %llu length %u samplerate %d channel
    // %d\n", getTimeStamp(), frame->timestamp,
    //       frame->length, frame->sample, frame->channels);
    TRTCAudioFrame audioframe;
    audioframe.data = (uint8_t *)frame->data;
    audioframe.length = frame->length;
    audioframe.sampleRate = frame->sample;
    audioframe.channel = frame->channels;
    audioframe.timestamp = frame->timestamp;
    audioframe.audioFormat = TRTCAudioFrameFormat::TRTCAudioFrameFormat_PCM;
    {
      std::lock_guard<std::mutex> lock(_mutexEngine);
      if (_tencentEngine) {
        _tencentEngine->sendCustomAudioData(&audioframe);
      }
    }
  } else {
    printf("do not send audio timestamp %llu length %u samplerate %d channel "
           "%d \n",
           frame->timestamp, frame->length, frame->sample, frame->channels);
  }
}

void TencentSDKWarper::onMediaStreamVideoDecode(TRTCDecodeVideoFrame *frame) {
  if (_bEnderRoom == true) {
    if (!m_av_flag) {
      return;
    }
    // printf("[%lu] send video timestamp %llu length %u width %u height %u\n",
    // getTimeStamp(), frame->timestamp,
    //       frame->length, frame->width, frame->height);
    TRTCVideoFrame videoframe;
    videoframe.data = (uint8_t *)frame->data;
    videoframe.bufferType = TRTCVideoBufferType::TRTCVideoBufferType_Buffer;
    videoframe.length = frame->length;
    videoframe.rotation = TRTCVideoRotation::TRTCVideoRotation0;
    videoframe.width = frame->width;
    videoframe.height = frame->height;
    videoframe.timestamp = frame->timestamp;
    videoframe.videoFormat = TRTCVideoFrameFormat::TRTCVideoFrameFormat_YUVI420;

    if (m_curwidth != frame->width || m_curheight != frame->height) {
      // 分辨率变化了需要设置编码参数。
      TRTCVideoEncParam param;
      param.videoFps = m_videofps;
      // 设置经验公式，具体也可以参考分辨率 码率 建议值 详见TRTCCloudDef.h
      // 文件。
      param.videoBitrate = frame->width * frame->height * 1.7 / 1000;
      std::lock_guard<std::mutex> lock(_mutexEngine);
      if (_tencentEngine) {
        _tencentEngine->setVideoEncoderParam(param);
        printf("\nwidth %d height %d\n", frame->width, frame->height);
      }

      m_curheight = frame->height;
      m_curwidth = frame->width;
    }

    {
      std::lock_guard<std::mutex> lock(_mutexEngine);
      if (_tencentEngine) {
        _tencentEngine->sendCustomVideoData(&videoframe);
      }
    }
  } else {
    printf("do not send video timestamp %llu length %u width %u height %u\n",
           frame->timestamp, frame->length, frame->width, frame->height);
  }
}

void TencentSDKWarper::onMediaPlayerNotifyEvent(int event, void *param) {
  printf("\n --------------  event %s(%d) --------------\n",
         getEventMsgByCode(event).c_str(), event);

  if (event == EVENT_MEDIAPLAY_PLAY_SUCCESS) {
    std::lock_guard<std::mutex> lock(_mutexEngine);
    if (_streamreader) {
      TRTCStreamInfo info;
      _streamreader->getFileInfo(&info);
      printf("--- duration %lld  sduration %s fps %d video width %d video "
             "height %d --\n",
             info.duration, info.str_duration, info.videoFps, info.videoWidth,
             info.videoheight);

      TRTCVideoEncParam param;
      m_videofps = info.videoFps;
      m_curheight = info.videoheight;
      m_curwidth = info.videoWidth;
      param.videoFps = info.videoFps;
      // 码率经验公式，供参考。
      param.videoBitrate = info.videoWidth * info.videoheight * 1.7 / 1000;
      if (_tencentEngine) {
        _tencentEngine->setVideoEncoderParam(param);
      }
    }
  }
}

/** C INTERFACE API **/
#ifdef __cplusplus
extern "C" {
#endif
#include "TencentSDKWarpper.h"

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"

static std::shared_ptr<TencentSDKWarper> sg_engine = NULL;
static std::thread sg_trtc_thread;
static std::string sg_user_id;
static std::string sg_user_sign;
static std::string sg_room_id;
static std::string sg_bussinfo;

static void _trtc_task_start(TRTCParams params, std::string file_path,
                             EncrytionMode mode, int av_flag) {
  printf("start testSingleRoom: roomName[%d]\n", params.roomId);
  //开始进房
  sg_engine = std::make_shared<TencentSDKWarper>(params.sdkAppId);
  sg_engine->setPushFile(file_path);
  sg_engine->setAVFlag(av_flag);
  sg_engine->start(params, mode);
  // printf("_trtc_task_start ------ sleep begin!\n");
  std::this_thread::sleep_for(std::chrono::seconds(2));
  // reset_call_status();
  // printf("_trtc_task_start ------ sleep end!\n");
}

int qcloud_iot_trtc_wrapper_init(void) {
  // 配置log路径
  // std::shared_ptr<LogCallback> log;
  // log.reset(new LogCallback());
  setLogDirPath("/tmp/log");
  setConsoleEnabled(true);
  /// 用跨进程方案必须调用该方法。
  // std::string core = "./TrtcCoreService";
  /// rootpath 过长会导致进程间通信建立失败，推荐设置为/tmp 或者 /data 目录。
  // std::string rootpath = "/tmp";
  // setIPCParam((char *)core.c_str(), (char *)rootpath.c_str());
  return 0;
}

#define STRING_ROOM
int qcloud_iot_trtc_wrapper_start(IOT_Trtc_Params params) {
  // 进房参数
  TRTCParams trtc_params;
  trtc_params.sdkAppId = params.sdk_app_id; // 进房参数设置 sdkappid
  sg_user_id = params.user_id;              // 进房参数设置 userid
  sg_user_sign = params.user_sig;           // 进房参数设置 usersig
  sg_room_id = params.str_room_id;
  trtc_params.userId = sg_user_id;
  trtc_params.userSig = sg_user_sign;

#ifdef STRING_ROOM
  sg_bussinfo = "{\"strGroupId\": \"" + sg_room_id + "\"}";
  trtc_params.roomId = -1;
  trtc_params.businessInfo = sg_bussinfo;
#else
  trtc_params.roomId = std::stoi(sg_room_id);
  trtc_params.businessInfo = sg_room_id;
#endif

  // set up thread
  sg_trtc_thread = std::thread(_trtc_task_start, trtc_params,
                               std::string((char *)params.user_data),
                               (EncrytionMode)params.mode, params.av_flag);
  return !sg_trtc_thread.joinable();
}

void qcloud_iot_trtc_wrapper_stop(void) {
  if (sg_engine) {
    sg_engine->stop();
    if (sg_trtc_thread.joinable()) {
      sg_trtc_thread.join();
    }
  }
}

int qcloud_iot_trtc_wrapper_usr_state(void) {
  int rc = 0;
  if (sg_engine) {
    rc = (true == sg_engine->getUsrRoomStat()) ? 1 : 0;
  }

  return rc;
}

#ifdef __cplusplus
}
#endif
