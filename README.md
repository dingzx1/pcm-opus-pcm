# Pcm-opus-pcm
Encoding and decoding of pcm files based on OPUS library
From \win32\VS2015\opus.sln enter the VS project. Regenerate the test_opus_encode and test_opus_decode solution, then the corresponding .exe is generated. The last, execute the .exe using cmd.
If it prompts that the corresponding cpp file does not exist, drag the corresponding cpp file in the file list into the solution.
Don't forget to modify the file path in the main function.
###########################################################################################################

Here is another encoding and decoding method based on ffmpeg command line applied to pcm-opus-pcm：
Notice：To run the ffmpeg tool, you must use cmd with administrator privileges.
pcm2opus：ffmpeg -ac 1 -ar 16000 -f s16le -i D:\ffmpeg_pcm\15vocal.pcm -acodec libopus -b:a 16k  D:\ffmpeg_pcm\15vocal.opus
opus2wav：ffmpeg -i D:\ffmpeg_pcm\15vocal_L0.opus -ac 1  -ar 16000 D:\ffmpeg_pcm\15vocal_L0.wav
opus2pcm：ffmpeg -i D:\ffmpeg_pcm\15vocal_L0.opus -ac 1  -ar 16000 -f s16le D:\ffmpeg_pcm\15vocal_L0_opus.pcm
