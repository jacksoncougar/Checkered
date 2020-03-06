//
// Created by root on 29/2/20.
//

#include "SoundSystem.h"
#include "Sound.h"

void Engine::SoundSystem::initialize() {
   
    

}

void Engine::SoundSystem::update(Engine::deltaTime) {

    auto sounds = Engine::getStore().getRoot().getComponentsOfType<Component::Sound>();
    for (auto sound : sounds)
    {
        Engine::log<module, Engine::high>("Playing sound ", sound->name);
        playSound();
        Engine::getStore().getRoot().eraseComponent<Component::Sound>(sound->getId());
        
    }
}




std::int32_t Engine::SoundSystem::convert_to_int(char* buffer, std::size_t len)
{
    std::int32_t a = 0;
    //if (std::endian::native == std::endian::little)
    std::memcpy(&a, buffer, len);
    // else
      //  for (std::size_t i = 0; i < len; ++i)
       //    reinterpret_cast<char*>(&a)[3 - i] = buffer[i];
    return a;
}

bool Engine::SoundSystem::load_wav_file_header(std::ifstream& file,
    std::uint8_t& channels,
    std::int32_t& sampleRate,
    std::uint8_t& bitsPerSample,
    ALsizei& size)
{
    char buffer[4];
    if (!file.is_open())
        return false;

    // the RIFF
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read RIFF" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "RIFF", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't begin with RIFF)" << std::endl;
        return false;
    }

    // the size of the file
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read size of file" << std::endl;
        return false;
    }

    // the WAVE
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read WAVE" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "WAVE", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (header doesn't contain WAVE)" << std::endl;
        return false;
    }

    // "fmt/0"
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read fmt/0" << std::endl;
        return false;
    }

    // this is always 16, the size of the fmt data chunk
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read the 16" << std::endl;
        return false;
    }

    // PCM should be 1?
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read PCM" << std::endl;
        return false;
    }

    // the number of channels
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read number of channels" << std::endl;
        return false;
    }
    channels = convert_to_int(buffer, 2);

    // sample rate
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read sample rate" << std::endl;
        return false;
    }
    sampleRate = convert_to_int(buffer, 4);

    // (sampleRate * bitsPerSample * channels) / 8
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read (sampleRate * bitsPerSample * channels) / 8" << std::endl;
        return false;
    }

    // ?? dafaq
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read dafaq" << std::endl;
        return false;
    }

    // bitsPerSample
    if (!file.read(buffer, 2))
    {
        std::cerr << "ERROR: could not read bits per sample" << std::endl;
        return false;
    }
    bitsPerSample = convert_to_int(buffer, 2);

    // data chunk header "data"
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data chunk header" << std::endl;
        return false;
    }
    if (std::strncmp(buffer, "data", 4) != 0)
    {
        std::cerr << "ERROR: file is not a valid WAVE file (doesn't have 'data' tag)" << std::endl;
        return false;
    }

    // size of data
    if (!file.read(buffer, 4))
    {
        std::cerr << "ERROR: could not read data size" << std::endl;
        return false;
    }
    size = convert_to_int(buffer, 4);

    /* cannot be at the end of file */
    if (file.eof())
    {
        std::cerr << "ERROR: reached EOF on the file" << std::endl;
        return false;
    }
    if (file.fail())
    {
        std::cerr << "ERROR: fail state set on the file" << std::endl;
        return false;
    }

    return true;
}

char* Engine::SoundSystem::load_wav(const std::string& filename,
    std::uint8_t& channels,
    std::int32_t& sampleRate,
    std::uint8_t& bitsPerSample,
    ALsizei& size)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open())
    {
        std::cerr << "ERROR: Could not open \"" << filename << "\"" << std::endl;
        return nullptr;
    }
    
    if (!load_wav_file_header(in, channels, sampleRate, bitsPerSample, size))
    {
        std::cerr << "ERROR: Could not load wav header of \"" << filename << "\"" << std::endl;
        return nullptr;
    }

    char* data = new char[size];

    in.read(data, size);

    return data;
}

int Engine::SoundSystem::playSound()
{
    {
        ALCdevice* openALDevice = alcOpenDevice(nullptr);
        if (!openALDevice)
            return 0;

        ALCcontext* openALContext;
        if (!alcCall(alcCreateContext, openALContext, openALDevice, openALDevice, nullptr) || !openALContext)
        {
            std::cerr << "ERROR: Could not create audio context" << std::endl;
            return 0;
        }
        ALCboolean contextMadeCurrent = false;
        if (!alcCall(alcMakeContextCurrent, contextMadeCurrent, openALDevice, openALContext)
            || contextMadeCurrent != ALC_TRUE)
        {
            std::cerr << "ERROR: Could not make audio context current" << std::endl;
            return 0;
        }

        std::uint8_t 	channels;
        std::int32_t 	sampleRate;
        std::uint8_t 	bitsPerSample;
        ALsizei			dataSize;
        char* rawSoundData = load_wav("Assets/Sounds/carHorn.wav", channels, sampleRate, bitsPerSample, dataSize);
        if (rawSoundData == nullptr || dataSize == 0)
        {
            std::cerr << "ERROR: Could not load wav" << std::endl;
            return 0;
        }
        std::vector<char> soundData(rawSoundData, rawSoundData + dataSize);

        ALuint buffer;
        alCall(alGenBuffers, 1, &buffer);

        ALenum format;
        if (channels == 1 && bitsPerSample == 8)
            format = AL_FORMAT_MONO8;
        else if (channels == 1 && bitsPerSample == 16)
            format = AL_FORMAT_MONO16;
        else if (channels == 2 && bitsPerSample == 8)
            format = AL_FORMAT_STEREO8;
        else if (channels == 2 && bitsPerSample == 16)
            format = AL_FORMAT_STEREO16;
        else
        {
            std::cerr
                << "ERROR: unrecognised wave format: "
                << channels << " channels, "
                << bitsPerSample << " bps" << std::endl;
            return 0;
        }

        alCall(alBufferData, buffer, format, soundData.data(), soundData.size(), sampleRate);
        soundData.clear(); // erase the sound in RAM

        ALuint source;
        alCall(alGenSources, 1, &source);
        alCall(alSourcef, source, AL_PITCH, 1);
        alCall(alSourcef, source, AL_GAIN, 1.0f);
        alCall(alSource3f, source, AL_POSITION, 0, 0, 0);
        alCall(alSource3f, source, AL_VELOCITY, 0, 0, 0);
        alCall(alSourcei, source, AL_LOOPING, AL_FALSE);
        alCall(alSourcei, source, AL_BUFFER, buffer);

        alCall(alSourcePlay, source);

        ALint state = AL_PLAYING;

        while (state == AL_PLAYING)
        {
            alCall(alGetSourcei, source, AL_SOURCE_STATE, &state);
        }

        alCall(alDeleteSources, 1, &source);
        alCall(alDeleteBuffers, 1, &buffer);

        alcCall(alcMakeContextCurrent, contextMadeCurrent, openALDevice, nullptr);
        alcCall(alcDestroyContext, openALDevice, openALContext);

        ALCboolean closed;
        alcCall(alcCloseDevice, closed, openALDevice, openALDevice);

        return 0;
    }
}