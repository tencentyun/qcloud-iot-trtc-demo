package com.engine;

import com.tencent.ITRTCAudioRecvCallback;
import com.tencent.TRTCAudioFrame;

import java.lang.ref.WeakReference;

/**
 * 音频回调接口的简单封装
 */
public class AudioRecvCallbackWarpper  extends ITRTCAudioRecvCallback {

    public EngineWarpper _callback;

    @Override
    public void onRecvAudioFrame(String userId, TRTCAudioFrame frame) {
        if (_callback != null){
            _callback.onRecvAudioFrame(userId, frame);
        }
    }
}
