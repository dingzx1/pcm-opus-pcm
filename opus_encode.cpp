#include <opus_types.h>
#include  <opus.h>
#include <cstring>
#include <memory>
#include <iostream>
#include<stdio.h>
#include <vector>
//#pragma warning(disable:4996);
#define DR_WAV_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
using namespace std;


#include "dr_wav.h"

#define FRAME_SIZE 160
#define MAX_FRAME_SIZE (6*FRAME_SIZE)

#define MAX_CHANNELS 1
#define MAX_PACKET_SIZE (3*1276)

#pragma pack(push)
#pragma pack(1)

struct WavInfo {
    uint16_t channels;
    uint32_t sampleRate;
    uint32_t bitsPerSample;
};

#pragma pack(pop)

#ifndef  nullptr
#define  nullptr NULL
#endif

class FileStream {
public:
    FileStream() {
        cur_pos = 0;
    }

    void Append(const char* data, size_t size) {
        if (cur_pos + size > Size()) {
            vec.resize(cur_pos + size);
        }
        memcpy(vec.data() + cur_pos, data, size);
        cur_pos += size;
    }

    void AppendU32(uint32_t val) {
        Append((char*)(&val), sizeof(val));
    }

    char* Data() {
        return vec.data();
    }

    size_t Size() {
        return vec.size();
    }

    size_t Read(void* buff, size_t elemSize, size_t elemCount) {
        size_t readed = std::min((vec.size() - cur_pos), (elemCount * elemSize)) / elemSize;
        if (readed > 0) {
            memcpy(buff, vec.data() + cur_pos, readed * elemSize);
            cur_pos += readed * elemSize;
        }
        return readed;
    }

    bool SeekCur(int offset) {
        if (cur_pos + offset > vec.size()) {
            cur_pos = !vec.empty() ? (vec.size() - 1) : 0;
            return false;
        }
        else {
            cur_pos += offset;
            return true;
        }
    }

    bool SeekBeg(int offset = 0) {
        cur_pos = 0;
        return SeekCur(offset);
    }

    bool WriteToFile(const char* filename) {
        FILE* fin = fopen(filename, "wb");
        if (!fin) {
            return false;
        }
        fseek(fin, 0, SEEK_SET);
        fwrite(vec.data(), sizeof(char), vec.size(), fin);
        fclose(fin);
        return true;
    }

    bool ReadFromFile(const char* filename) {
        FILE* fin = fopen(filename, "rb");
        if (!fin) {
            return false;
        }
        fseek(fin, 0, SEEK_END);
        long fileSize = ftell(fin);
        vec.resize(static_cast<unsigned long long int>(fileSize));
        fseek(fin, 0, SEEK_SET);
        fread(vec.data(), sizeof(char), vec.size(), fin);
        fclose(fin);
        return true;
    }

private:
    std::vector<char> vec;
    size_t cur_pos;
};

bool wavWrite_int16(char* filename, int16_t* buffer, int sampleRate, uint32_t totalSampleCount) {
    drwav_data_format format = {};
    format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
    format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = 1;
    format.sampleRate = (drwav_uint32)sampleRate;
    format.bitsPerSample = 16;
    drwav* pWav = drwav_open_file_write(filename, &format);
    if (pWav) {
        drwav_uint64 samplesWritten = drwav_write(pWav, totalSampleCount, buffer);
        drwav_uninit(pWav);
        if (samplesWritten != totalSampleCount) {
            fprintf(stderr, "ERROR\n");
            return false;
        }
        return true;
    }
    return false;
}

int16_t* wavRead_int16(char* filename, uint32_t* sampleRate, uint64_t* totalSampleCount) {
    unsigned int channels;
    int16_t* buffer = drwav_open_and_read_file_s16(filename, &channels, sampleRate, totalSampleCount);
    if (buffer == nullptr) {
        fprintf(stderr, "ERROR\n");
        return nullptr;
    }
    if (channels != 1) {
        drwav_free(buffer);
        buffer = nullptr;
        *sampleRate = 0;
        *totalSampleCount = 0;
    }
    return buffer;
}


bool pcm2stream(char* input, FileStream* output) {
    uint32_t sampleRate = 16000;
    uint64_t totalSampleCount = 0;
    int16_t* wavBuffer = wavRead_int16(input, &sampleRate, &totalSampleCount);
    if (wavBuffer == nullptr) return false;
    WavInfo info = {};
    info.bitsPerSample = 16;
    info.sampleRate = sampleRate;
    info.channels = 1;
    output->SeekBeg();
    output->Append((char*)&info, sizeof(info));
    output->Append((char*)wavBuffer, totalSampleCount * sizeof(int16_t));
    free(wavBuffer);
    return true;
}


// bool pcm2Opus(FileStream* input, FileStream* output) {
bool pcm2Opus(char* in_file, char* out_file) {
    // WavInfo in_info = {};
    // uint32_t bitsPerSample = in_info.bitsPerSample;
    // uint32_t sampleRate = in_info.sampleRate;
    // uint16_t channels = in_info.channels;
    uint32_t bitsPerSample = 16;
    uint32_t sampleRate = 16000;
    uint16_t channels = 1;
    int err = 0;
    if (channels > MAX_CHANNELS) {
        return false;
    }
    OpusEncoder* encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_AUDIO, &err);
    if (!encoder || err < 0) {
        fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
        if (!encoder) {
            opus_encoder_destroy(encoder);
        }
        return false;
    }
    FILE* fp;
    int size = 0;
    opus_int16 pcm_bytes[FRAME_SIZE * MAX_CHANNELS];
    // fp = fopen("D:/wj/tmp/2021_11_22/39_opus/1.pcm", "rb");
    fp = fopen(in_file, "rb");
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    int n_sample = size / 2;
    // short* data1 = new short[size1 / 2];
    uint16_t* data = new uint16_t[size / 2];
    fread(data, 1, size, fp);
    fclose(fp);
    FILE* fp_out = fopen(out_file, "wb");


    // const uint16_t *data = (uint16_t *) (input->Data() + sizeof(in_info));
    //  uint16_t* data = (uint16_t*)data1;
    // uint16_t* data = data1;
    // size_t size = (input->Size()) / 2;
    // size = size1 / 2;
    // printf("n_sample = %d\n", n_sample);
    //opus_int16 pcm_bytes[FRAME_SIZE * MAX_CHANNELS];
    size_t index = 0;
    size_t step = static_cast<size_t>(FRAME_SIZE * channels);
    // FileStream encodedData;
    unsigned char cbits[MAX_PACKET_SIZE];
    size_t frameCount = 0;
    size_t readCount = 0;
    while (index < n_sample) {
        printf("\r%d / %d", index, n_sample);
        memset(&pcm_bytes, 0, sizeof(pcm_bytes));
        if (index + step <= n_sample) {
            memcpy(pcm_bytes, data + index, step * sizeof(uint16_t));
            index += step;
        }
        else {
            readCount = n_sample - index;
            memcpy(pcm_bytes, data + index, (readCount) * sizeof(uint16_t));
            index += readCount;
        }
        int nbBytes = opus_encode(encoder, pcm_bytes, channels * FRAME_SIZE, cbits, MAX_PACKET_SIZE);
        if (nbBytes < 0) {
            fprintf(stderr, "encode failed: %s\n", opus_strerror(nbBytes));
            break;
        }
        ++frameCount;
        // encodedData.AppendU32(static_cast<uint32_t>(nbBytes));
        // encodedData.Append((char*)cbits, static_cast<size_t>(nbBytes));
        fwrite(&nbBytes, sizeof(int), 1, fp_out);
        fwrite(cbits, 1, nbBytes, fp_out);
    }
    printf("\n");
    // output->Append(encodedData.Data(), encodedData.Size());
    opus_encoder_destroy(encoder);
    delete[] data;
    fclose(fp_out);
    return true;
}


void splitpath(const char* path, char* drv, char* dir, char* name, char* ext) {
    const char* end;
    const char* p;
    const char* s;
    if (path[0] && path[1] == ':') {
        if (drv) {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    }
    else if (drv)
        *drv = '\0';
    for (end = path; *end && *end != ':';)
        end++;
    for (p = end; p > path && *--p != '\\' && *p != '/';)
        if (*p == '.') {
            end = p;
            break;
        }
    if (ext)
        for (s = end; (*ext = *s++);)
            ext++;
    for (p = end; p > path;)
        if (*--p == '\\' || *p == '/') {
            p++;
            break;
        }
    if (name) {
        for (s = p; s < end;)
            *name++ = *s++;
        *name = '\0';
    }
    if (dir) {
        for (s = path; s < p;)
            *dir++ = *s++;
        *dir = '\0';
    }
}

void wav2opus(char* in_file, char* out_file) {
    pcm2Opus(in_file, out_file);
}

int main() {
    char* in_file = "C:/Users/dingzx1/Desktop/OPUS/opus-pcm/1.pcm";
    char drive[3];
    char dir[256];
    char fname[256];
    char ext[256];
    char out_file[1024];
    splitpath(in_file, drive, dir, fname, ext);
    if (memcmp(".pcm", ext, strlen(ext)) == 0) {
        sprintf(out_file, "%s%s%s.out", drive, dir, fname);
        wav2opus(in_file, out_file);
        printf("encode_done. Decode file: %s\n", out_file);
    }
    return 0;
}
