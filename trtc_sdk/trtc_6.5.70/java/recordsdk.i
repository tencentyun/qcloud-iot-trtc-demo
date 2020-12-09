//
// 功能说明：
//   SWIG 工具可以将c++接口形式的TRTCEngine转成其它语言 java 和 go 的接口封装。
//
// 2019-10-15 @ arvinwu

%module(directors="1") trtcenginewarper

// 虚类导出，可以用于目标语言的类继承。
%feature("director") ITRTCCloudCallback;
%feature("director") ITRTCVideoRecvCallback;
%feature("director") ITRTCAudioRecvCallback;
%feature("director") ITRTCDecryptionCallback;
%feature("director") ITRTCEncryptionCallback;
%feature("director") ITRTCLogCallback;
%feature("director") ITRTCMediaRecorderCallback;
%feature("director") ITRTCMediaMixerCallback;

// "%{" 和 “}%” 的内容原样输出到转换后的 c++ 文件中
%{

#include <string>
#include <stdint.h>
#include "../include/TRTCCloud.h"
#include "../include/TRTCCloudCallback.h"
#include "../include/ITRTCMediaMixer.h"
#include "../include/ITRTCMediaRecorder.h"
#include "../include/TRTCCloudDef.h"
#include "../include/TXLiteAVCode.h"

%}

// swig 解析和转换
%include "typemaps.i"
%include "std_string.i"
%include "stdint.i"
%include "various.i"
%include "arrays_java.i"

%include <enumtypesafe.swg>
%javaconst(1);

%typemap(jni) char *BYTE "jbyteArray"
%typemap(jtype) char *BYTE "byte[]"
%typemap(jstype) char *BYTE "byte[]"
%typemap(in) char *BYTE {
  $1 = (uint8_t *) JCALL2(GetByteArrayElements, jenv, $input, 0);
  int length = jenv->GetArrayLength($input);
  if (arg1->data) {
     memcpy(arg1->data, $1, length);
     JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
     return;
  }else{
     JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
     return;
  }
}

%typemap(out) char *BYTE
%{
    if($1 && arg1->length>0){
        jresult = jenv->NewByteArray(arg1->length);
        jthrowable e = jenv->ExceptionOccurred();
        if(e){
            jenv->Throw(e);
            jenv->ExceptionClear();

            jresult = 0;
        }else{
            jenv->SetByteArrayRegion(jresult, 0, arg1->length, (const jbyte*)$1);
            jthrowable err = jenv->ExceptionOccurred();
            if(err){
                jenv->Throw(err);
                jenv->ExceptionClear();
            }
        }
    }else{
        jresult = 0;
    }
%}

%typemap(argout) char *BYTE {
  JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
}

%typemap(javain) char *BYTE "$javainput"
%typemap(javaout) char *BYTE {
    return $jnicall;
}

/* Prevent default freearg typemap from being used */
%typemap(freearg) char *BYTE ""

%apply char* BYTE {uint8_t* data};

/* ENCRYPT_DATA typemap*/
%typemap(jni) char *ENCRYPT_DATA "jbyteArray"
%typemap(jtype) char *ENCRYPT_DATA "byte[]"
%typemap(jstype) char *ENCRYPT_DATA "byte[]"
%typemap(in) char *ENCRYPT_DATA {
  $1 = (uint8_t *) JCALL2(GetByteArrayElements, jenv, $input, 0);
  int length = jenv->GetArrayLength($input);
  if (arg1->EncryptedData) {
     memcpy(arg1->EncryptedData, $1, length);
     JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
     return;
  }else{
     JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
     return;
  }
}

%typemap(out) char *ENCRYPT_DATA
%{
    if($1 && arg1->EncryptedDataLength>0){
        jresult = jenv->NewByteArray(arg1->EncryptedDataLength);
        jthrowable e = jenv->ExceptionOccurred();
        if(e){
            jenv->Throw(e);
            jenv->ExceptionClear();

            jresult = 0;
        }else{
            jenv->SetByteArrayRegion(jresult, 0, arg1->EncryptedDataLength, (const jbyte*)$1);
            jthrowable err = jenv->ExceptionOccurred();
            if(err){
                jenv->Throw(err);
                jenv->ExceptionClear();
            }
        }
    }else{
        jresult = 0;
    }
%}

%typemap(argout) char *ENCRYPT_DATA {
  JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
}

%typemap(javain) char *ENCRYPT_DATA "$javainput"
%typemap(javaout) char *ENCRYPT_DATA {
    return $jnicall;
}

/* Prevent default freearg typemap from being used */
%typemap(freearg) char *ENCRYPT_DATA ""

%apply char* ENCRYPT_DATA {uint8_t* EncryptedData};


/* UN_ENCRYPT_DATA typemap*/
%typemap(jni) char *UN_ENCRYPT_DATA "jbyteArray"
%typemap(jtype) char *UN_ENCRYPT_DATA "byte[]"
%typemap(jstype) char *UN_ENCRYPT_DATA "byte[]"
%typemap(in) char *UN_ENCRYPT_DATA {
  $1 = (uint8_t *) JCALL2(GetByteArrayElements, jenv, $input, 0);
  int length = jenv->GetArrayLength($input);
  if (arg1->unEncryptedData) {
     memcpy(arg1->unEncryptedData, $1, length);
     JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
     return;
  }else{
     JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
     return;
  }
}

%typemap(out) char *UN_ENCRYPT_DATA
%{
    if($1 && arg1->unEncryptedDataLength>0){
        jresult = jenv->NewByteArray(arg1->unEncryptedDataLength);
        jthrowable e = jenv->ExceptionOccurred();
        if(e){
            jenv->Throw(e);
            jenv->ExceptionClear();

            jresult = 0;
        }else{
            jenv->SetByteArrayRegion(jresult, 0, arg1->unEncryptedDataLength, (const jbyte*)$1);
            jthrowable err = jenv->ExceptionOccurred();
            if(err){
                jenv->Throw(err);
                jenv->ExceptionClear();
            }
        }
    }else{
        jresult = 0;
    }
%}

%typemap(argout) char *UN_ENCRYPT_DATA {
  JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *) $1, 0);
}

%typemap(javain) char *UN_ENCRYPT_DATA "$javainput"
%typemap(javaout) char *UN_ENCRYPT_DATA {
    return $jnicall;
}

/* Prevent default freearg typemap from being used */
%typemap(freearg) char *UN_ENCRYPT_DATA ""

%apply char* UN_ENCRYPT_DATA {uint8_t* unEncryptedData};


%include "../include/TRTCCloud.h"
%include "../include/TRTCCloudCallback.h"
%include "../include/ITRTCMediaMixer.h"
%include "../include/ITRTCMediaRecorder.h"
%include "../include/TRTCCloudDef.h"
%include "../include/TXLiteAVCode.h"

%extend TRTCVideoFrame{
    void initData(int data_length_init){
        if ($self->data) {
            delete[] $self->data;
        }
        $self->data = new uint8_t[data_length_init];
    }
    void uninitData(int data_length_uninit){
        if ($self->data) {
            delete[] $self->data;
        }
    }
};

%extend  TRTCAudioFrame{
    void initData(int data_length_init){
        if ($self->data) {
            delete[] $self->data;
        }
        $self->data = new uint8_t[data_length_init];
    }
    void uninitData(int data_length_uninit){
        if ($self->data) {
            delete[] $self->data;
        }
    }
};

%extend  TRTCCustomEncryptionData {
    ~TRTCCustomEncryptionData(){
        if ($self->is_encryption_buffer_setted && $self->EncryptedData) {
            delete[] $self->EncryptedData;
        }
        if ($self->is_decryption_buffer_setted && $self->unEncryptedData) {
            delete[] $self->unEncryptedData;
        }
    }
    void initEncryptionData(int data_length_init){
        if ($self->is_encryption_buffer_setted && $self->EncryptedData) {
            delete[] $self->EncryptedData;
        }
        $self->EncryptedData = new uint8_t[data_length_init];
        $self->is_encryption_buffer_setted = true;
    }
    
    void initUnEncryptionData(int data_length_init){
        if ($self->is_decryption_buffer_setted && $self->unEncryptedData) {
            delete[] $self->unEncryptedData;
        }
        $self->unEncryptedData = new uint8_t[data_length_init];
        $self->is_decryption_buffer_setted = true;
    }
};

// 重命名接口首字母大写
%rename("%(firstuppercase)s", %$isfunction) "";

