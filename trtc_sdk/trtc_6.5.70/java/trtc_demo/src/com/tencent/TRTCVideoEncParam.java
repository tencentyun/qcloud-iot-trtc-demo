/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.10
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.tencent;

public class TRTCVideoEncParam {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected TRTCVideoEncParam(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(TRTCVideoEncParam obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        trtcenginewarperJNI.delete_TRTCVideoEncParam(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setVideoResolution(TRTCVideoResolution value) {
    trtcenginewarperJNI.TRTCVideoEncParam_videoResolution_set(swigCPtr, this, value.swigValue());
  }

  public TRTCVideoResolution getVideoResolution() {
    return TRTCVideoResolution.swigToEnum(trtcenginewarperJNI.TRTCVideoEncParam_videoResolution_get(swigCPtr, this));
  }

  public void setResMode(TRTCVideoResolutionMode value) {
    trtcenginewarperJNI.TRTCVideoEncParam_resMode_set(swigCPtr, this, value.swigValue());
  }

  public TRTCVideoResolutionMode getResMode() {
    return TRTCVideoResolutionMode.swigToEnum(trtcenginewarperJNI.TRTCVideoEncParam_resMode_get(swigCPtr, this));
  }

  public void setEncType(TRTCVideoEncodeType value) {
    trtcenginewarperJNI.TRTCVideoEncParam_encType_set(swigCPtr, this, value.swigValue());
  }

  public TRTCVideoEncodeType getEncType() {
    return TRTCVideoEncodeType.swigToEnum(trtcenginewarperJNI.TRTCVideoEncParam_encType_get(swigCPtr, this));
  }

  public void setVideoFps(long value) {
    trtcenginewarperJNI.TRTCVideoEncParam_videoFps_set(swigCPtr, this, value);
  }

  public long getVideoFps() {
    return trtcenginewarperJNI.TRTCVideoEncParam_videoFps_get(swigCPtr, this);
  }

  public void setVideoBitrate(long value) {
    trtcenginewarperJNI.TRTCVideoEncParam_videoBitrate_set(swigCPtr, this, value);
  }

  public long getVideoBitrate() {
    return trtcenginewarperJNI.TRTCVideoEncParam_videoBitrate_get(swigCPtr, this);
  }

  public TRTCVideoEncParam() {
    this(trtcenginewarperJNI.new_TRTCVideoEncParam(), true);
  }

}
