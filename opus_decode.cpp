
#include <opus_types.h>
#include  <opus.h>
#include <cstring>
#include <memory>

#include <vector>
#define DR_WAV_IMPLEMENTATION
#include <iostream>
using namespace std;

#define FRAME_SIZE 160
#define MAX_FRAME_SIZE (6*FRAME_SIZE)

#define MAX_CHANNELS 1
#define MAX_PACKET_SIZE (3*1276)
#pragma pack(push)
#pragma pack(1)

#pragma pack(pop)
#ifndef  nullptr
#define  nullptr NULL
#endif
class FileStream {
public:
    FileStream() {
        cur_pos = 0;
    }

    void Append(const char *data, size_t size) {
        if (cur_pos + size > Size()) {
            vec.resize(cur_pos + size);
        }
        memcpy(vec.data() + cur_pos, data, size);
        cur_pos += size;
    }

    void AppendU32(uint32_t val) {
        Append((char *) (&val), sizeof(val));
    }

    char *Data() {
        return vec.data();
    }

    size_t Size() {
        return vec.size();
    }

    size_t Read(void *buff, size_t elemSize, size_t elemCount) {
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
        } else {
            cur_pos += offset;
            return true;
        }
    }
    bool SeekBeg(int offset = 0) {
        cur_pos = 0;
        return SeekCur(offset);
    }

    bool WriteToFile(const char *filename) {
        FILE *fin = fopen(filename, "wb");
        if (!fin) {
            return false;
        }
        fseek(fin, 0, SEEK_SET);
        fwrite(vec.data(), sizeof(char), vec.size(), fin);
        fclose(fin);
        return true;
    }

    bool ReadFromFile(const char *filename) {
        FILE *fin = fopen(filename, "rb");
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
bool Opus2pcm(char *in_file, char *out_file) {
    uint32_t bitsPerSample = 16;
    uint32_t sampleRate = 16000;
    uint16_t channels = 1;
    int nbBytes;
    int err = 0;
    OpusDecoder *decoder = opus_decoder_create(sampleRate, channels, &err);
    if (!decoder || err < 0) {
        fprintf(stderr, "failed to create decoder: %s\n", opus_strerror(err));
        if (!decoder) {
            opus_decoder_destroy(decoder);
        }
        return false;
    }
    FILE* fp;
    int size = 0;
    opus_int16 pcm_bytes[FRAME_SIZE * MAX_CHANNELS*sizeof(opus_int16)];
    fp = fopen(in_file, "rb");
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    unsigned char *data = new unsigned char[size];
    fread(data, 1, size, fp);
    fclose(fp);
    FILE* fp_out = fopen(out_file, "wb");   
    opus_int16 out[MAX_FRAME_SIZE * MAX_CHANNELS];
    size_t index = 0;
    unsigned char cbits[MAX_PACKET_SIZE];
    size_t frameCount = 0;
    size_t readCount = 0;
    
    while (index < size) {
        printf("\r%d / %d", index, size);
        memset(&cbits, 0, MAX_PACKET_SIZE);
        memcpy(&nbBytes, data + index, sizeof(int));
        // printf("nbBytes=%d\n", nbBytes);
        index += sizeof(int);
        if (index + nbBytes <= size) {
          memcpy(cbits, data + index, nbBytes);
          index += nbBytes;
        } else {
          readCount = size - index;
          memcpy(cbits, data + index, readCount);
          index += readCount;
        }
        int frame_size = opus_decode(decoder, cbits, nbBytes, out, MAX_FRAME_SIZE, 0);
        if (frame_size < 0) {
            fprintf(stderr, "decode failed: %s\n", opus_strerror(frame_size));
            break;
        }
        ++frameCount;
        fwrite(out, sizeof(opus_int16), frame_size, fp_out);
    }
    printf("\n");
    opus_decoder_destroy(decoder);
    delete[] data;
    fclose(fp_out);
    return true;
    
}
void splitpath(const char *path, char *drv, char *dir, char *name, char *ext) {
    const char *end;
    const char *p;
    const char *s;
    if (path[0] && path[1] == ':') {
        if (drv) {
            *drv++ = *path++;
            *drv++ = *path++;
            *drv = '\0';
        }
    } else if (drv)
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
int main() {

    char *in_file ="C:/Users/dingzx1/Desktop/OPUS/opus-pcm/1.out";
    char drive[3];
    char dir[256];
    char fname[256];
    char ext[256];
    char out_file[1024];
    splitpath(in_file, drive, dir, fname, ext);
    if (memcmp(".out", ext, strlen(ext)) == 0) {
        sprintf(out_file, "%s%s%sdecode.pcm", drive, dir, fname);
        Opus2pcm(in_file, out_file);
        printf("decode_done. Decoded file: %s\n", out_file);
    }  
    printf("press any key to exit.\n");
    return 0;
}
