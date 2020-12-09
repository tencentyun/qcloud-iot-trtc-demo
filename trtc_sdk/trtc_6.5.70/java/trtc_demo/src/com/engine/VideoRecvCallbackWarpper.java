package com.engine;

import com.tencent.ITRTCVideoRecvCallback;
import com.tencent.TRTCVideoFrame;
import com.tencent.TRTCVideoStreamType;

import java.lang.ref.WeakReference;

/**
 * 视频回调接口的简单封装。
 */
public class VideoRecvCallbackWarpper extends ITRTCVideoRecvCallback {
    public EngineWarpper _callback;
    @Override
    public void onRecvVideoFrame(String userId, TRTCVideoStreamType streamType, TRTCVideoFrame frame) {
        if (_callback != null){
            _callback.onRecvVideoFrame(userId, streamType, frame);
        }
    }
}
