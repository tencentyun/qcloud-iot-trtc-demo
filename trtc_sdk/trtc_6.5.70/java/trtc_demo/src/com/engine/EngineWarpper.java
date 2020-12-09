package com.engine;


import java.io.*;
import java.lang.ref.WeakReference;
import java.math.BigInteger;
import java.util.Arrays;

import com.tencent.*;
import com.tencent.StringBuffer;

/**
 * 房间引擎包装类。
 */
public class EngineWarpper  extends ITRTCCloudCallback {
    private TRTCParams    _selfInfo;            // 进房参数
    private File            _dump;              // 音频文件dump
    private DataOutputStream _bufferWriter;     // 音频文件dump

    private AudioRecvCallbackWarpper _audio;    // 房间里其它人的音频回调
    private VideoRecvCallbackWarpper _video;    // 房间里其它人的视频回调

    private ITRTCCloud    _room;                // TRTC房间

    private String       _first_user;

    /**
     * 加载动态库。
     */
    public static void init(){
        /**
         * 单进程版本
         * trtcenginewarper 依赖 libTrtcEngine.so 在 SDK压缩包的 lib目录下
         */
        System.loadLibrary("trtcenginewarper");

        /**
         * 多进程版本
         * trtcengineipcwarper 依赖 libTrtcEngineIPC.so 在 SDK压缩包的 lib目录下
         * libTrtcEngineIPC.so 会执行 TrtcCoreService 程序 在SDK压缩吧的 bin目录下
         * TrtcCoreService 依赖 libTrtcEngine.so
         */
//        System.loadLibrary("trtcengineipcwarper");
    }

    public EngineWarpper(){
        super();
        try {
            _dump = new File("dump.pcm");
            if (!_dump.exists())
            {
                _dump.createNewFile();
            }

            _bufferWriter = new DataOutputStream(new FileOutputStream(_dump.getName(), false));
        }catch (Exception e){
            e.printStackTrace();
        }


        StringBuffer _userid = new StringBuffer();
        StringBuffer _usersig = new StringBuffer();
        _selfInfo = new TRTCParams();
        String userid = "client_42";
        String usersig = "eAFNjlFPgzAURv8Lz0ZLWxBMfNgI0ylEw9yMT02lBS9sBUo32Yz-3UpYsseek-Pd-jhvyeqa53mzV4aZYyudOwc5VyMGIZWBAqS2MN*CfTGKJ8nbFgTjhhEtLppe1GxUlrkUITcIiO9PjRxa0JLxwoyT2AsxQudrB6l7aJTtsK0QwcS6szSw*-*Z691iavdcb1rsobQ4jdfRclFXGUT*XA3dKWwyfmOahD5CeVB51W2eX3cfcutTIWHxPYN4VpMgfdk-nIRuFS6Wc-*L0Pd0JcuQD9HxKUG4*1zHXhVl*b3z*weI9lmG";

        _userid.setBuffer(userid);
        _userid.setBuffer_len(userid.length());
        _usersig.setBuffer(usersig);
        _usersig.setBuffer_len(usersig.length());
        int _sdkappid = 1400188366;
        _selfInfo.setSdkAppId(_sdkappid);
        _selfInfo.setRoomId(601);
        _selfInfo.setUserId(_userid);
        _selfInfo.setUserSig(_usersig);
        _selfInfo.setClientRole(TRTCClientRole.TRTCClientRole_Audience);

        _room = trtcengine.createInstance(_sdkappid);
        if (_room == null){
            return;
        }
        _room.setCallback(this);

        _audio = new AudioRecvCallbackWarpper();
        _audio._callback = this;

        _video = new VideoRecvCallbackWarpper();
        _video._callback = this;


    }

    /**
     * 业务流程开始
     * @return 0 成功
     */
    public int start(){
        if (_room != null){
            _room.enterRoom(_selfInfo, TRTCAppScene.TRTCAppSceneVideoCall);
        }
        return 0;
    }

    /**
     * 业务流程结束
     * @return 0 成功
     */
    public int stop(){
        if (_room != null){
            _room.exitRoom();
        }
        try {
            if (_bufferWriter != null){
                _bufferWriter.close();
            }
        } catch (Exception e){
            e.printStackTrace();
        }

        return 0;
    }

    /**
     * 音频数据回调
     * @param userId 房间中其他人的id
     * @param frame 音频帧
     */
    public void onRecvAudioFrame(String userId, TRTCAudioFrame frame) {
        System.out.println("onRecvAudioFrame " + frame.getLength());

        if (_first_user == null){
            _first_user = userId;
        }

        if (_first_user.equals(userId) ) {
            byte[] a = frame.getData();
            try {
                _bufferWriter.write(a);
            } catch (Exception e) {

            }
        }
    }

    /**
     * 视频数据回调
     * @param userId        房间中其他人id
     * @param streamType    流类型，主流/辅流
     * @param frame         视频帧
     */
    public void onRecvVideoFrame(String userId, TRTCVideoStreamType streamType, TRTCVideoFrame frame) {
        System.out.println("onRecvVideoFrame " + frame.getLength() + " w:" + frame.getWidth() +" h:"+frame.getHeight());
        System.out.println(frame.getData());
    }

    @Override
    public void onConnectionLost() {
        /**
         * 网络断开通知
         */
    }

    @Override
    public void onConnectionRecovery() {
        /**
         * 网络重连成功通知
         */
    }

    @Override
    public void onTryToReconnect() {
        /**
         * 网络重连中通知
         */
    }

    @Override
    public void onEnterRoom(BigInteger elapsed) {
        /**
         * 进房成功回调这里。参数是进房耗时，单位ms
         */
        System.out.println("onEnterRoom use " + elapsed + " ms");
    }

    @Override
    public void onError(TXLiteAVError errCode, String errMsg, SWIGTYPE_p_void arg) {
        /**
         * 内部发生错误。做退房处理。
         */
        System.out.println("onError code " + errCode + " msg " + errMsg);
    }

    @Override
    public void onWarning(TXLiteAVWarning warningCode, String warningMsg, SWIGTYPE_p_void arg) {
        System.out.println("onWarning code " + warningCode + " msg " + warningMsg);
    }

    @Override
    public void onExitRoom(int reason) {
        /**
         * 退房成功回调。
         */
        System.out.println("onExitRoom ");
    }

    @Override
    public void onFirstAudioFrame(String userId) {
        /**
         * 收到房间某人的首个音频。
         */
    }

    @Override
    public void onFirstSubStreamFrame(String userId, long width, long height) {
        /**
         * 收到房间某人的首个辅流视频。
         */
    }

    @Override
    public void onFirstVideoFrame(String userId, long width, long height) {
        /**
         * 收到房间默认首个主流视频。
         */
    }

    @Override
    public void onMissCustomCmdMsg(String userId, int cmdId, int errCode, int missed) {

    }

    @Override
    public void onRecvCustomCmdMsg(String userId, int cmdId, int seq, SWIGTYPE_p_unsigned_char msg, int msglen) {

    }

    @Override
    public void onRecvSEIMsg(String userId, SWIGTYPE_p_unsigned_char data, int datalen) {

    }


    @Override
    public void onUserAudioAvailable(String userId, boolean available) {
        /**
         * 房间中其它人 音频上行状态变化 有->没有 available=false， 没有->有 available=true。
         */
    }

    @Override
    public void onUserVideoAvailable(String userId, boolean available) {
        /**
         * 房间中其它人 视频上行状态变化 有->没有 available=false， 没有->有 available=true。
         */
    }

    @Override
    public void onUserSubStreamAvailable(String userId, boolean available) {
        /**
         * 房间中其它人 辅流视频上行状态变化 有->没有 available=false， 没有->有 available=true。
         */
    }

    @Override
    public void onUserEnter(String userId) {
        /**
         * 房间中其它人进房就会触发此回调。
         */
        System.out.println("onUserEnter " + userId);
        if (_room != null){
            _room.setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat.TRTCAudioFrameFormat_PCM, _audio);
            _room.setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat.TRTCVideoFrameFormat_YUVI420, _video);
        }
    }

    @Override
    public void onUserExit(String userId, int reason) {
        /**
         * 房间中其它人退房/没有了音视频上行 就会触发此回调。
         */
        if (_room != null){
            _room.setRemoteAudioRecvCallback(userId, TRTCAudioFrameFormat.TRTCAudioFrameFormat_Unknown, null);
            _room.setRemoteVideoRecvCallback(userId, TRTCVideoFrameFormat.TRTCVideoFrameFormat_Unknown, null);
        }
    }
}