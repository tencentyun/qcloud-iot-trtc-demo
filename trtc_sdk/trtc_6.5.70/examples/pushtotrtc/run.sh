#!/bin/bash
#
# 参数说明
#  sdkAppId     应用ID
#  demoSences   1. 推本地MP4文件；2.单流录制；4.混流录制。
#  userId       用户ID
#  userSig      用户凭证
#  roomId       房间号
#  recDir       录制文件的存放主目录。demoSences 取2或4有效
#  recFiles     1. 纯音频文件； 2.纯视频文件；4.音视频文件。demoSences 取2或4有效
#  mp4pathMain  本地主流Mp4文件路径。demoSences 取1有效
#  mp4pathAux   本地辅流Mp4文件路径。demoSences 取1有效
#
#  sdkAppId + userId + userSig 是进入实时音视频房间的钥匙。如果是测试需要可以联系客服人员。


#
# 演示场景1. 推送本地MP4文件。
#
#
./demo --sdkAppId 1400188366 --demoSences 1 --userId client_41 --userSig eAFNjl1vgjAYRv9Lb7cs-ZCCuyMgzq2g0plMbxqEgs0USKlsavbf1xFJdtlzct4*N-DO*FOW5825NsJcWgmeAQSPA1aFrI0qldQW5kdlX2KC7jJrW1WIzAiii39NV3yKQVmGJhAizyOU3hv53SotRVaa4SR2phjC8bde6k41te2wrSDBxLpRGnX6W4Yc6iLi4Om4olOVxfFsGyzWAWeH6OytdhXPP4L9nFxf4ofQYTpcXV836f4I5zxJaQrTyl8c-Ddql7ELSTqz3EUxlKfEr5A727oR5v1yg-qvkoVrVDYY-PwC7i1W*w__ --roomId 601 --recDir="./record" --recFiles=7 --mp4pathMain="../resource/ruguo-640x360.mp4"

#
# 演示场景2. 单流录制
#
#
./demo --sdkAppId 1400188366 --demoSences 2 --userId client_42 --userSig eAFNjtFOgzAUQP*FV42WAgVMfFCcDgMqGWTRlwZoYd0GbUq3dTP**zrCEh-vOTn33l8rTxZ3ZV3zXa*wOgpqPVjAuh0xI7RXrGFUGlhvmZmwCydZCsEILhV2JPnXDGSDR2WY7QJgB4GD0NRQLZikuGzUuBJ6IQTgem1P5cB4bzpoKuBAx7irVKy7fGZ7yLcdD4b2tHFgrcHprIjiLJIJexO5jlWYZ4tdWYm0WHZHMtef7366DtEJLbMDb55e1CFuyUew6vx7n-D2a-usq2Ll3kRrvcmTOUfwp*1i15t9R6-Vfni0-s6CElmX --roomId 601 --recDir="./record" --recFiles=7

#
# 演示场景3. 混流录制
#
#
./demo --sdkAppId 1400188366 --demoSences 4 --userId client_43 --userSig eAFNjl1PgzAUQP9LXzGmtHwMEx-YMhOGhmysbvDSsNJhhUEHrdMZ-7uVsMTHe07OvfcbbJ-T*4KxTreKqi-JwQOA4G7EouStEkfBewNZI8xEHTzJQkpR0kJR3Jf-mqGs6agMsx0I7dkMe97U8E8pek6LoxpXIjdAEN6uffB*EF1rOmQqiBE27iaVOP19Zrueb2MXBfa0cRCVwS9LsojWi8wioRNuahzud2T9pM*JdckDHzEp0GHrZ3Gb6bQ-EP1WRVU8X*0zFS2bdznXuWSvTnfZJWlMmjpZaetUnfPrsLlKHHSP4OcXOGtZuQ__ --roomId 601 --recDir="./record" --recFiles=7 



