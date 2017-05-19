#define NOMINMAX
#include "sound.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <memory>

Sound::Sound()
{
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::stringstream ss;
        ss << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error(ss.str());
    }
}

Sound::~Sound()
{
    PaError err = Pa_Terminate();
    if (err != paNoError)
        std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;

}

std::shared_ptr<OggVorbis_File> vorbisFile;

int Sound::play(const std::string& filename)
{
    vorbisFile = std::make_shared<OggVorbis_File>();
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) {
        std::cout << "Sound::play error: file not found: " << filename << std::endl;
        return 0;
    }
    if (ov_open(file, vorbisFile.get(), nullptr, 0) < 0) {
          std::cout << "Sound::play error: input does not appear to be an Ogg bitstream." << std::endl;
          return 0;
    }

    vorbis_info* vorbisInfo = ov_info(vorbisFile.get(), -1);

    PaStream* stream = nullptr;
    PaError err = Pa_OpenDefaultStream(&stream,
        0,                              // no input channels
        vorbisInfo->channels,           // stereo output
        paFloat32,                      // 32 bit floating point output
        vorbisInfo->rate,
        paFramesPerBufferUnspecified,   // frames per buffer, i.e. the number
                                        //  of sample frames that PortAudio will
                                        //  request from the callback. Many apps
                                        //  may want to use
                                        //  paFramesPerBufferUnspecified, which
                                        //  tells PortAudio to pick the best,
                                        //  possibly changing, buffer size.
        &Sound::callback,               // this is your callback function
        vorbisFile.get());              // This is a pointer that will be passed to
                                        //  your callback
    if (err != paNoError) {
        std::stringstream ss;
        ss << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error(ss.str());
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::stringstream ss;
        ss << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error(ss.str());
    }

    m_streams[m_streamIDNext++] = stream;
    
    return m_streamIDNext - 1;
}

void Sound::stop(int streamID)
{
    auto stream = m_streams.find(streamID);
    if (stream == m_streams.end())
        return;

    PaError err = Pa_StopStream(stream->second);
    if (err != paNoError) {
        std::stringstream ss;
        ss << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        throw std::runtime_error(ss.str());
    }
}

int Sound::callback(const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    OggVorbis_File* vorbisFile = static_cast<OggVorbis_File*>(userData);
    float* out = static_cast<float*>(outputBuffer);
    
    int samples = 0;
    while(samples < framesPerBuffer) {
        float** vorbisOut = nullptr;
        int current_section = 0;
        long ret = ov_read_float(vorbisFile, &vorbisOut, framesPerBuffer - samples, &current_section);
        if (!ret) {
            break; // EOF
        } else if (ret < 0) {
            if (ret == OV_EBADLINK) {
                std::cout << "Sound::callback error: Corrupt bitstream section!" << std::endl;
                break;
            }
        }

        for(int i=0; i<ret; i++)
        {
            *out++ = vorbisOut[0][i];
            *out++ = vorbisOut[1][i];
        }

        samples += ret;
    }

    return paNoError;
}