+----------------------+
 speex 回音消除测试程序
+----------------------+

speex 是一套针对语音的开源免费无专利保护的音频压缩库。
同时 speex 也包含了音频降噪、AGC、回音消除等音频处理的算法

speextest 在 speex 音频算法库的基础上实现了回音消除的功能


编译与运行
----------
音频播放和录制都是使用的 win32 的 wavapi 接口，因此需要在 msys2 + gcc 环境下编译

先编译 speexdsp 库：
./build-speexdsp.sh

再编译测试程序：
./build.sh

运行测试程序：
./test.exe

运行后，程序会播放 test.pcm 的音频，同时会录音
如果回音消除起作用了，录下来的声音是听不到播放的 test.pcm 的声音的

实测回音消除的效果还是很不错的



rockcarry
2021-7-15