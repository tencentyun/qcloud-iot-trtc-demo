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
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include "TRTCCloud.h"
#include "usages/TencentSDKWapperPushMp4.h" // 推Mp4本地文件（必须带音轨，否则异常）
#include "usages/TencentSDKWapperRecord.h" // 单流录制
#include "usages/TencentSDKWapperMixRecord.h"  // 混流录制

#define OUTPUT_LOG // 控制主函数的输出。

constexpr int DEMO_SENCE_PUSH = 1;
constexpr int DEMO_SENCE_RECORD = 2;
constexpr int DEMO_SENCE_MIXREC = 4;

/* 程序运行参数 管理结构体 */
typedef struct InputArgs{
    bool           isRunning;       // 处理 TERM 信号 收到 TERM 信号会置为 false 程序退出
    uint32_t        sdkAppId;        // 【必须】
    std::string     userId;          // 【必须】
    std::string     userSig;         // 【必须】
    std::string     roomId;          // 【必须】
    uint32_t        demoSences;     // 位控制：推流 1 单流录制 2 混流录制 4，如 3表示，推流 + 单流录制 两个场景会先后执行。
    uint32_t        recFiles;       // 位控制：纯音频 1 纯视频 2 音视频 4，如 3表示， 生成2个文件 一个是纯音频一个是纯视频。
    std::string     mp4pathMain;    //  推流主音视频文件（必须带音轨）
    std::string     mp4pathAux;     // 推流辅路音视频文件（必须带音轨）
    std::string     recDir;         // 录制文件存放目录
    
    InputArgs()
    :isRunning(false)
    ,sdkAppId(0)
    ,userId("")
    ,userSig("")
    ,roomId("")
    ,demoSences(0)
    ,recFiles(OUTFILE_TYPE_AUDIO_VIDEO)
    ,mp4pathMain("")
    ,mp4pathAux("")
    ,recDir("./record"){
    }
    
    void printArgs();
    
    bool checkArgs(){
        if (sdkAppId == 0){
            return false;
        }
        
        if (userId.empty()){
            return false;
        }
        
        if (userSig.empty()){
            return false;
        }
        
        if (roomId.empty()){
            return false;
        }
        
        if (demoSences < 1 || demoSences > 7){
            return false;
        }
        return true;
    }
    
}InputArgs;

InputArgs g_input_arg_info;

void InputArgs::printArgs(){
#ifdef OUTPUT_LOG
    printf("ARGS:   --sdkAppId = %d\n", sdkAppId);
    printf("        --userId = %s\n", userId.c_str());
    printf("        --userSig = %s\n", userSig.c_str());
    printf("        --recDir = %s\n", recDir.c_str());
    printf("        --demoSences = %d\n", demoSences);
    printf("        --roomId = %s\n", roomId.c_str());
    printf("        --recFiles = %d\n", recFiles);
    printf("        --mp4pathMain = %s\n", mp4pathMain.c_str());
    printf("        --mp4pathAux = %s\n", mp4pathAux.c_str());
#endif
}

/* 参数解析 */
char* l_opt_arg = nullptr;
const char* short_options = "a:u:s:r:t:F::M::A::R::"; // a: 带一个":"表示带参数值  S:: 带两个":"表示是可选参数。 不带":"表示有参无值。
struct option long_options[] = {
    { "sdkAppId",       required_argument,   NULL,    'a'     },
    { "userId",         required_argument,   NULL,    'u'     },
    { "userSig",        required_argument,   NULL,    's'     },
    { "roomId",         required_argument,   NULL,    'r'     },
    { "demoSences",     required_argument,   NULL,    't'     },
    { "recFiles",       optional_argument,   NULL,    'F'     },
    { "mp4pathMain",    optional_argument,   NULL,    'M'     },
    { "mp4pathAux",     optional_argument,   NULL,    'A'     },
    { "recDir",         optional_argument,   NULL,    'R'     },
    {      0,     0,     0,     0}, // 【必须】全0 item 结尾。
};

void print_usage(){
#ifdef OUTPUT_LOG
    printf("DESCRIPTION\n");
    printf("         show tencent trtc sdk functions, such as push mp4, record stream and stream mix-record.\n\n");
    printf("  --sdkAppId, -a\n");
    printf("         a number like 1400XXXXXX.\n\n");
    printf("  --userId, -u\n");
    printf("         a str like teacher_01 stand for a teacher.\n\n");
    printf("  --userSig, -s\n");
    printf("         a str like _KDU#DDDSKJFSK_*DSKSDFKDSF... stand for a token, witch relates with sdkAppId,userId. \n\n");
    printf("  --roomId, -r\n");
    printf("         a number like 66666 stand for a room.\n\n");
    printf("  --demoSences, -t\n");
    printf("         a number in [1,7] stand for witch demos you want to run, 1 for push, 2 for record stream, 4 for stream mix-record.\n\n");
    printf("  --recFiles, -F\n");
    printf("         if demoSences [2,7] must be setted. a number in [1,7] stand for files want to record, 1 for audio only, 2 for video only, 4 for audio and video\n\n");
    printf("  --mp4pathMain, -M\n");
    printf("         if demoSences is in {1,3,5,7} must be setted. a str stand for a mp4 file path to be pushed to TRTC, the mp4 must cantain audio track.\n\n");
    printf("  --mp4pathAux, -A\n");
    printf("         if demoSences is in {1,3,5,7} must be setted. a str stand for a mp4 file path to be pushed to TRTC, the mp4 must cantain audio track.\n\n");
    printf("  --recDir, -R\n");
    printf("         if demoSences [2,7] must be setted. a str stand for where to put recorded files.\n\n");
#endif
}

void print_arg(int argc, char* argv[]){
#ifdef OUTPUT_LOG
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
#endif
}

int parse_arg(int argc, char* argv[]){
    print_arg(argc, argv);
    int c;
    std::string opt;
    while((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'a':
                opt = optarg;
                g_input_arg_info.sdkAppId = std::stoi(opt);
                break;
            case 'u':
                opt = optarg;
                g_input_arg_info.userId = opt;
                break;
            case 's':
                opt = optarg;
                g_input_arg_info.userSig = opt;
                break;
            case 'r':
                opt = optarg;
                g_input_arg_info.roomId = opt;
                break;
            case 't':
                opt = optarg;
                g_input_arg_info.demoSences = std::stoi(opt);
                break;
            case 'F':
                opt = optarg;
                g_input_arg_info.recFiles = std::stoi(opt);
                break;
            case 'M':
                opt = optarg;
                g_input_arg_info.mp4pathMain = opt;
                break;
            case 'A':
                opt = optarg;
                g_input_arg_info.mp4pathAux = opt;
                break;
            case 'R':
                opt = optarg;
                g_input_arg_info.recDir = opt;
                break;
            default:
                return -1;
        }
    }
    
    if (false == g_input_arg_info.checkArgs()) {
        return -2;
    }
    
    return 0;
}


// 处理USR1 自定义信号。
void onSIGUSR1_Stop(int sig){
    printf("receive sig SIGUSR1(%d) stop record!\n",sig);
    g_input_arg_info.isRunning = false;
}

// 处理终止程序的信号，可以响应 kill -TERM PID 命令，顺利退出程序。
void onSIGTERM_Stop(int sig){
    printf("receive sig SIGTERM(%d) stop record!\n",sig);
    g_input_arg_info.isRunning = false;
}

// 推流，房间所有人退出房间，结束推流。
void Push() {
    printf(" stream push begin \n");
    std::string roomName = g_input_arg_info.roomId;
    
    // step 1 进房参数
    TRTCParams params;
    params.sdkAppId = g_input_arg_info.sdkAppId;      // 进房参数设置 sdkappid
    std::string userid = g_input_arg_info.userId;     // 进房参数设置 userid
    std::string usersig = g_input_arg_info.userSig;   // 进房参数设置 usersig
    params.userId = userid;
    params.roomId = std::stoi(roomName);
    params.userSig = usersig;
    params.businessInfo = roomName;
    params.clientRole = TRTCClientRole::TRTCClientRole_Anchor; // 【必须是主播 否则无法推流】
    
    // step 2 开始进房
    std::shared_ptr<TencentSDKWarperPushMp4> inst = std::make_shared<TencentSDKWarperPushMp4>(params.sdkAppId);
    inst->start(params, g_input_arg_info.mp4pathMain.c_str(), g_input_arg_info.mp4pathAux.c_str());
    g_input_arg_info.isRunning = true;
    int reason = 0;
    while (g_input_arg_info.isRunning && inst->isRunning(reason)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // step 3 退房
    inst->stop();
    printf(" stream push end with reason %d \n", reason);
}

// 所有人退出房间，结束
void Record() {
    printf(" stream record begin \n");
    std::string roomName = g_input_arg_info.roomId;
    
    
    // step 1 进房参数
    TRTCParams params;
    params.sdkAppId = g_input_arg_info.sdkAppId;      // 进房参数设置 sdkappid
    std::string userid = g_input_arg_info.userId;     // 进房参数设置 userid
    std::string usersig = g_input_arg_info.userSig;   // 进房参数设置 usersig
    params.userId = userid;
    params.roomId = std::stoi(roomName);
    params.userSig = usersig;
    params.businessInfo = roomName;
    params.clientRole = TRTCClientRole::TRTCClientRole_Audience; // 【必须】 用观众角色。
    
    // step 2 开始进房
    std::shared_ptr<TencentSDKWarperRecord> inst = std::make_shared<TencentSDKWarperRecord>(params.sdkAppId);
    inst->start(params,roomName.c_str(), g_input_arg_info.recFiles, g_input_arg_info.recDir.c_str());

    g_input_arg_info.isRunning = true;
    int reason = 0;
    while (g_input_arg_info.isRunning && inst->isRunning(reason)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // step 3 退房
    inst->stop();
    
    printf(" stream record end with reason %d\n", reason);
}

// 所有人退出房间，结束
void MixRecord() {
    printf(" streams mix record begin \n");
    std::string roomName = g_input_arg_info.roomId;
    
    // step 1 进房参数
    TRTCParams params;
    params.sdkAppId = g_input_arg_info.sdkAppId;      // 进房参数设置 sdkappid
    std::string userid = g_input_arg_info.userId;     // 进房参数设置 userid
    std::string usersig = g_input_arg_info.userSig;   // 进房参数设置 usersig
    std::string busiinfo = "";
    params.userId = userid;
    params.roomId = std::stoi(roomName);
    params.userSig = usersig;
    params.businessInfo = busiinfo;
    params.clientRole = TRTCClientRole::TRTCClientRole_Audience; // 【必须】 观众角色。
    
    
    // step 2 开始进房
    std::shared_ptr<TencentSDKWarperMixRecord> inst = std::make_shared<TencentSDKWarperMixRecord>(params.sdkAppId);
    // 混流录制的文件 文件名含mixer关键字。
    inst->start(params, roomName.c_str(), "mixer", g_input_arg_info.recFiles, g_input_arg_info.recDir.c_str());
    
    g_input_arg_info.isRunning = true;
    int reason = 0;
    while (g_input_arg_info.isRunning && inst->isRunning(reason)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // step 3 退房
    inst->stop();
    printf(" stream mix record end with reason %d \n", reason);
}

void MainProcess() {
    if (g_input_arg_info.demoSences & DEMO_SENCE_PUSH) {
        Push(); // 集成推流功能，看这里
    }
 
    if (g_input_arg_info.demoSences & DEMO_SENCE_RECORD) {
        Record(); // 集成分流录制，看这里
    }
    
    if (g_input_arg_info.demoSences & DEMO_SENCE_MIXREC) {
        MixRecord(); // 集成混流录制，看这里
    }
}

int main(int argc, char* argv[]) {
    signal(SIGUSR1, onSIGUSR1_Stop);
    signal(SIGTERM, onSIGTERM_Stop);
    
    
    if (0 != parse_arg(argc, argv)) {
        print_usage();
        printf("parse input args failed!\n");
        return 1;
    }
    
    g_input_arg_info.printArgs();
    
    /**
     * 日志默认存放在 /tmp 目录下对应的roomid和userid 的目录下。
     */
    char logpath[1024] = {0};
    snprintf(logpath, 1024, "/tmp/%s_%s",g_input_arg_info.roomId.c_str(), g_input_arg_info.userId.c_str());
    setLogDirPath(logpath);
    
    /**
     * 如果不想控制台输出日志，请将true，改为false。
     */
    setConsoleEnabled(true);
    
    /**
     * 逻辑总入口，对接从这里开始看。
     * 里面分3个场景的演示。
     * - 推流
     * - 分流录制
     * - 混流录制
     */
    MainProcess();
    
    /**
     * 延迟1s退出让事件上报飞一会儿。
     */
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return 0;
}


