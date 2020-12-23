## 安装依赖库

```bash
apt-get install zlib1g zlib1g-dev
```

## 编译

1. 执行如下命令，开始编译：

    `./cmake_build.sh all`

2. 执行如下命令，运行demo;
    ```bash
    cd output/bin/
    ./trtc_sample
    ```

## 运行

TRTC demo运行步骤：
1. 修改 `trtc_sample` 同级目录下的 `device_info.json` 文件三元组信息为 IoT Explorer 平台创建的设备信息
2. 运行 `trtc_sample` 后出现二维码，使用腾讯连连小程序或者APP扫码进行绑定；
3. 按回车后控制台出现如下界面：

```
0: device active request video call!
1: device active request audio call!
2: device active disable call!
3: exit!
```
> 说明：demo 启动后，可以自动响应 APP 的呼叫，也可以主动呼叫 APP。

在命令窗口输入：
```
0：表示设备端主动视频呼叫APP;
1：表示设备端主动音频呼叫APP;
2：表示设备端主动挂断；
3：表示退出demo;
```

## 说明
TRTC demo 说明
1. TRTC demo 向 APP 推送本地 mp4 文件中的音视频流；
2. 在TRTC demo的同级目录下，名称：ruguo-640x360.mp4
3. 可以接收 APP 的音视频流，以文件的形式保存在trtc_sample同级目录下。
- 音频文件：audio_file.pcm（PCM格式，一次通话）
- 视频文件：yuv_tmp.yuv(YUV420 PLANNER格式，仅一帧)
