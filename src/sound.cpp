#include <string.h>
#include <stdint.h>
#include <dlib/log.h>
#include <dlib/index_pool.h>
#include <dlib/math.h>
#include "sound.h"

#if defined(__MACH__)
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif

#include <AL/alut.h>

namespace dmSound
{
    struct SoundData
    {
        SoundDataType m_Type;
        const void*   m_Data;

        // Index in m_SoundData
        uint16_t      m_Index;

        // AL_FORMAT_MONO8, AL_FORMAT_MONO16, AL_FORMAT_STEREO8, AL_FORMAT_STEREO16
        ALenum        m_Format;
        // Size in bytes
        ALsizei       m_Size;
        ALfloat       m_Frequency;
    };

    struct SoundInstance
    {
        uint32_t m_CurrentBufferOffset;
        uint16_t m_Index;
        uint16_t m_SoundDataIndex;
        uint16_t m_SourceIndex;
        uint16_t m_BufferIndices[2];
        float    m_Gain;
        uint32_t m_Looping : 1;
    };

    struct SoundSystem
    {
        dmArray<SoundInstance> m_Instances;
        dmIndexPool16          m_InstancesPool;

        dmArray<SoundData>     m_SoundData;
        dmIndexPool16          m_SoundDataPool;

        dmArray<ALuint>        m_Buffers;
        dmIndexPool32          m_BuffersPool;

        dmArray<ALuint>        m_Sources;
        dmArray<ALint>         m_SourceStates;

        float                  m_MasterGain;
        uint32_t               m_BufferSize;
    };

    SoundSystem* g_SoundSystem = 0;

    void CheckAndPrintError()
    {
        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
        {
            dmLogError("%s", alGetString(error));
        }
        else
        {
            error = alutGetError();
            if (error != ALUT_ERROR_NO_ERROR )
            {
                printf("FOO\n");
                dmLogError("%s", alutGetErrorString(error));
            }
        }
    }

    void SetDefaultInitializeParams(InitializeParams* params)
    {
        memset(params, 0, sizeof(*params));
        params->m_MasterGain = 1.0f;
        params->m_MaxSoundData = 128;
        params->m_MaxSources = 16;
        params->m_MaxBuffers = 32;
        params->m_BufferSize = 4 * 4096;
        params->m_MaxInstances = 256;
    }

    Result Initialize(dmConfigFile::HConfig config, const InitializeParams* params)
    {
        if (!alutInit(0, 0))
        {
            CheckAndPrintError();
            dmLogError("Failed to initialize sound");
            return RESULT_UNKNOWN_ERROR;
        }

        float master_gain = params->m_MasterGain;

        g_SoundSystem = new SoundSystem();
        SoundSystem* sound = g_SoundSystem;

        uint32_t max_sound_data = params->m_MaxSoundData;
        uint32_t max_buffers = params->m_MaxBuffers;
        uint32_t max_sources = params->m_MaxSources;
        uint32_t max_instances = params->m_MaxInstances;

        if (config)
        {
            master_gain = dmConfigFile::GetFloat(config, "sound.gain", 1.0f);
            max_sound_data = (uint32_t) dmConfigFile::GetInt(config, "sound.max_sound_data", (int32_t) max_sound_data);
            max_buffers = (uint32_t) dmConfigFile::GetInt(config, "sound.max_buffers", (int32_t) max_buffers);
            max_sources = (uint32_t) dmConfigFile::GetInt(config, "sound.max_sources", (int32_t) max_sources);
            max_instances = (uint32_t) dmConfigFile::GetInt(config, "sound.max_instances", (int32_t) max_instances);
        }

        sound->m_Instances.SetCapacity(max_instances);
        sound->m_Instances.SetSize(max_instances);
        sound->m_InstancesPool.SetCapacity(max_instances);
        for (uint32_t i = 0; i < max_instances; ++i)
        {
            sound->m_Instances[i].m_Index = 0xffff;
            sound->m_Instances[i].m_SoundDataIndex = 0xffff;
            sound->m_Instances[i].m_SourceIndex = 0xffff;
        }

        sound->m_SoundData.SetCapacity(max_sound_data);
        sound->m_SoundData.SetSize(max_sound_data);
        sound->m_SoundDataPool.SetCapacity(max_sound_data);
        for (uint32_t i = 0; i < max_sound_data; ++i)
        {
            sound->m_SoundData[i].m_Index = 0xffff;
        }

        sound->m_Buffers.SetCapacity(max_buffers);
        sound->m_Buffers.SetSize(max_buffers);
        sound->m_BuffersPool.SetCapacity(max_buffers);

        sound->m_Sources.SetCapacity(max_sources);
        sound->m_Sources.SetSize(max_sources);
        sound->m_SourceStates.SetCapacity(max_sources);
        sound->m_SourceStates.SetSize(max_sources);

        sound->m_MasterGain = master_gain;
        sound->m_BufferSize = params->m_BufferSize;

        for (uint32_t i = 0; i < max_sources; ++i)
        {
            alGenSources (1, &sound->m_Sources[i]);
            CheckAndPrintError();
            alGetSourcei (sound->m_Sources[i], AL_SOURCE_STATE, &sound->m_SourceStates[i]);
            CheckAndPrintError();
        }

        for (uint32_t i = 0; i < max_buffers; ++i)
        {
            alGenBuffers(1, &sound->m_Buffers[i]);
            CheckAndPrintError();
        }

        return RESULT_OK;
    }

    Result Finalize()
    {
        Result result = RESULT_OK;

        if (g_SoundSystem)
        {
            SoundSystem* sound = g_SoundSystem;
            if (sound->m_SoundDataPool.Size() > 0)
            {
                dmLogError("%d sound-data not deleted", sound->m_SoundDataPool.Size());
                result = RESULT_RESOURCE_LEAK;
            }

            if (sound->m_InstancesPool.Size() > 0)
            {
                dmLogError("%d sound-instances not deleted", sound->m_InstancesPool.Size());
                result = RESULT_RESOURCE_LEAK;
            }

            alSourceStopv(sound->m_Sources.Size(), &sound->m_Sources[0]);
            for (uint32_t i = 0; i < sound->m_Sources.Size(); ++i)
            {
                alSourcei(sound->m_Sources[i], AL_BUFFER, AL_NONE);
            }
            alDeleteSources(sound->m_Sources.Size(), &sound->m_Sources[0]);

            alutExit();
            delete sound;
            g_SoundSystem = 0;
        }
        return result;
    }

    Result NewSoundData(const void* sound_buffer, uint32_t sound_buffer_size, SoundDataType type, HSoundData* sound_data)
    {
        ALenum format;
        ALsizei size;
        ALfloat frequency;

        const void* buffer = alutLoadMemoryFromFileImage(sound_buffer, sound_buffer_size, &format, &size, &frequency);
        if (!buffer)
        {
            CheckAndPrintError();
            return RESULT_UNKNOWN_ERROR;
        }

        SoundSystem* sound = g_SoundSystem;

        if (sound->m_SoundDataPool.Remaining() == 0)
        {
            *sound_data = 0;
            return RESULT_OUT_OF_INSTANCES;
        }
        uint16_t index = sound->m_SoundDataPool.Pop();

        SoundData* sd = &sound->m_SoundData[index];
        sd->m_Index = index;
        sd->m_Data = buffer;
        sd->m_Format = format;
        sd->m_Size = size;
        sd->m_Frequency = frequency;
        *sound_data = sd;

        return RESULT_OK;
    }

    Result DeleteSoundData(HSoundData sound_data)
    {
        free((void*) sound_data->m_Data);

        SoundSystem* sound = g_SoundSystem;
        sound->m_SoundDataPool.Push(sound_data->m_Index);
        sound_data->m_Index = 0xffff;

        return RESULT_OK;
    }

    Result NewSoundInstance(HSoundData sound_data, HSoundInstance* sound_instance)
    {
        SoundSystem* ss = g_SoundSystem;
        if (ss->m_InstancesPool.Remaining() == 0)
        {
            *sound_instance = 0;
            return RESULT_OUT_OF_INSTANCES;
        }

        uint16_t index = ss->m_InstancesPool.Pop();
        SoundInstance* si = &ss->m_Instances[index];
        assert(si->m_Index == 0xffff);

        si->m_CurrentBufferOffset = 0;
        si->m_SoundDataIndex = sound_data->m_Index;
        si->m_Index = index;
        si->m_SourceIndex = 0xffff;
        si->m_BufferIndices[0] = 0xffff;
        si->m_BufferIndices[1] = 0xffff;
        si->m_Gain = 1.0f;
        si->m_Looping = 0;

        *sound_instance = si;

        return RESULT_OK;
    }

    Result DeleteSoundInstance(HSoundInstance sound_instance)
    {
        SoundSystem* ss = g_SoundSystem;
        uint16_t index = sound_instance->m_Index;
        ss->m_InstancesPool.Push(index);
        sound_instance->m_Index = 0xffff;
        sound_instance->m_SoundDataIndex = 0xffff;

        return RESULT_OK;
    }

    static uint32_t FillBuffer(SoundData* sound_data, SoundInstance* instance, ALuint buffer)
    {
        SoundSystem* sound = g_SoundSystem;
        assert(instance->m_CurrentBufferOffset <= (uint32_t) sound_data->m_Size);
        uint32_t to_buffer = dmMath::Min(sound->m_BufferSize, sound_data->m_Size - instance->m_CurrentBufferOffset);

        if (instance->m_Looping && to_buffer == 0)
        {
            instance->m_CurrentBufferOffset = 0;
            to_buffer = dmMath::Min(sound->m_BufferSize, sound_data->m_Size - instance->m_CurrentBufferOffset);
        }

        const char* p = (const char*) sound_data->m_Data;
        p += instance->m_CurrentBufferOffset;
        alBufferData(buffer, sound_data->m_Format, p, to_buffer, sound_data->m_Frequency);

        instance->m_CurrentBufferOffset += to_buffer;

        //printf("BUFFER... %d\n", to_buffer);
        return to_buffer;
    }

    Result Update()
    {
        SoundSystem* sound = g_SoundSystem;

        for (uint32_t i = 0; i < sound->m_Sources.Size(); ++i)
        {
            //ALint prev_state = sound->m_SourceStates[i];
            alGetSourcei (sound->m_Sources[i], AL_SOURCE_STATE, &sound->m_SourceStates[i]);
            //if (prev_state != sound->m_SourceStates[i] )
              //  printf("Changed state: %x -> %x\n", prev_state, sound->m_SourceStates[i]);
            CheckAndPrintError();
        }

        for (uint32_t i = 0; i < sound->m_Instances.Size(); ++i)
        {
            SoundInstance* instance = &sound->m_Instances[i];

            if (instance->m_SourceIndex == 0xffff)
                break;

            SoundData* sound_data = &sound->m_SoundData[instance->m_SoundDataIndex];

            ALuint source = sound->m_Sources[instance->m_SourceIndex];

            ALint state;
            alGetSourcei (source, AL_SOURCE_STATE, &state);
            CheckAndPrintError();
            if (state != AL_PLAYING && !instance->m_Looping)
            {
                // Instance done playing
                assert(instance->m_BufferIndices[0] != 0xffff);
                assert(instance->m_BufferIndices[1] != 0xffff);
                sound->m_BuffersPool.Push(instance->m_BufferIndices[0]);
                sound->m_BuffersPool.Push(instance->m_BufferIndices[1]);

                instance->m_BufferIndices[0] = 0xffff;
                instance->m_BufferIndices[1] = 0xffff;

                instance->m_SourceIndex = 0xffff;
            }
            else
            {
                // Buffer more data
                int processed;
                alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);
                while (processed > 0)
                {
                    ALuint buffer;
                    alSourceUnqueueBuffers(source, 1, &buffer);
                    CheckAndPrintError();

                    uint32_t to_buffer = FillBuffer(sound_data, instance, buffer);

                    if (to_buffer > 0)
                    {
                        alSourceQueueBuffers(source, 1, &buffer);
                        CheckAndPrintError();
                    }
                    --processed;
                }
            }
        }

        return RESULT_OK;
    }

    Result Play(HSoundInstance sound_instance)
    {
        assert(sound_instance->m_SourceIndex == 0xffff && "TODO: Stop...?");

        SoundSystem* sound = g_SoundSystem;
        if (sound->m_BuffersPool.Remaining() < 2)
        {
            dmLogWarning("Out of sound buffers.");
            return RESULT_OUT_OF_BUFFERS;
        }

        uint32_t index = 0xffffffff;
        for (uint32_t i = 0; i < sound->m_SourceStates.Size(); ++i)
        {
            if (sound->m_SourceStates[i] != AL_PLAYING)
            {
                index = i;
                alSourcei(sound->m_Sources[i], AL_BUFFER, AL_NONE);
                break;
            }
        }

        if (index == 0xffffffff)
        {
            dmLogWarning("Out of sound sources");
            return RESULT_OUT_OF_SOURCES;
        }

        sound_instance->m_SourceIndex = index;
        ALuint source = sound->m_Sources[index];

        alSourcef(source, AL_GAIN, sound_instance->m_Gain * sound->m_MasterGain);
        CheckAndPrintError();
        sound->m_SourceStates[index] = AL_PLAYING;

        SoundData* sound_data = &sound->m_SoundData[sound_instance->m_SoundDataIndex];

        uint32_t buf_index1 = sound->m_BuffersPool.Pop();
        uint32_t buf_index2 = sound->m_BuffersPool.Pop();

        assert(sound_instance->m_BufferIndices[0] == 0xffff);
        assert(sound_instance->m_BufferIndices[1] == 0xffff);

        sound_instance->m_BufferIndices[0] = buf_index1;
        sound_instance->m_BufferIndices[1] = buf_index2;

        ALuint buf1 = sound->m_Buffers[buf_index1];
        ALuint buf2 = sound->m_Buffers[buf_index2];

        uint32_t to_buffer1 = FillBuffer(sound_data, sound_instance, buf1);
        (void) to_buffer1;
        uint32_t to_buffer2 = FillBuffer(sound_data, sound_instance, buf2);

        alSourceQueueBuffers(source, 1, &buf1);
        CheckAndPrintError();
        if (to_buffer2 > 0)
        {
            alSourceQueueBuffers(source, 1, &buf2);
            CheckAndPrintError();
        }

        alSourcePlay(source);
        CheckAndPrintError();

        return RESULT_OK;
    }

    Result Stop(HSoundInstance sound_instance)
    {
        SoundSystem* sound = g_SoundSystem;
        sound_instance->m_Looping = 0;
        if (sound_instance->m_SourceIndex != 0xffff)
        {
            ALuint source = sound->m_Sources[sound_instance->m_SourceIndex];
            alSourceStop(source);
            CheckAndPrintError();
            // NOTE: sound_instance->m_SourceIndex will be set to 0xffff in Update when state != AL_PLAYING
        }
        return RESULT_OK;
    }

    bool IsPlaying(HSoundInstance sound_instance)
    {
        return sound_instance->m_SourceIndex != 0xffff;
    }

    Result SetLooping(HSoundInstance sound_instance, bool looping)
    {
        sound_instance->m_Looping = (uint32_t) looping;
        return RESULT_OK;
    }
}
