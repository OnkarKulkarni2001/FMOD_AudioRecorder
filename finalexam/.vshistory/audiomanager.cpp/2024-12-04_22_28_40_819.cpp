#include "audiomanager.h"

#include <Windows.h>
#ifdef PlaySound
#undef PlaySound
#endif

#include <random>

#include <iostream>
#include <fmod/fmod_errors.h>
#include <fstream>

void CheckError(FMOD_RESULT result, const char* file, int line)
{
	if (result != FMOD_OK)
	{
		printf("FMOD Error [%d, %s]: '%s' at %d\n", static_cast<int>(result), FMOD_ErrorString(result), file, line);
	}
}

AudioManager::AudioManager()
{
}

AudioManager::~AudioManager()
{
}

void AudioManager::Initialize()
{
	if (m_IsInitialized)
		return;

	FMOD_RESULT result = FMOD::System_Create(&m_System);
	if (result != FMOD_OK)
	{
		printf("Failed to create the FMOD System!\n");
		return;
	}

	result = m_System->init(32, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK)
	{
		printf("Failed to initialize the audio system!\n");
		result = m_System->close();
		if (result != FMOD_OK)
		{
			printf("Failed to close system!\n");
		}
		return;
	}

	m_ExInfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
	m_ExInfo.numchannels = 2;
	m_ExInfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
	m_ExInfo.defaultfrequency = 44100;
	m_ExInfo.length = m_ExInfo.defaultfrequency * m_ExInfo.numchannels * sizeof(float) * 5;

	// TODO: Create DSPs
	// STore in m_DSPs
	CreateDSP(FMOD_DSP_TYPE_DISTORTION);
	CreateDSP(FMOD_DSP_TYPE_CHORUS);
	CreateDSP(FMOD_DSP_TYPE_DELAY);
	CreateDSP(FMOD_DSP_TYPE_CONVOLUTIONREVERB);

	m_IsInitialized = true;
}

void AudioManager::Destroy()
{
	if (!m_IsInitialized)
		return;

	FMOD_RESULT result = FMOD_OK;

	result = m_Channel->stop();

	result = m_RecordingSound->release();
	FMODCheckError(result);

	result = m_System->release();
	FMODCheckError(result);

	m_IsInitialized = false;
}

void AudioManager::Update()
{
	if (!m_IsInitialized)
		return;

	FMOD_RESULT result = m_System->update();
	FMODCheckError(result);

	if (m_IsRecording)
	{
		ProcessRecording();
	}
}

void AudioManager::CreateDSP(FMOD_DSP_TYPE m_DSP)
{
	FMOD::DSP* newDSP = nullptr;
	FMOD_RESULT result = m_System->createDSPByType(m_DSP, &newDSP);
	
	FMODCheckError(result);
	m_DSPs.push_back(newDSP);
}

void AudioManager::SetActiveDSP(unsigned int m_ActiveDSP)
{
	if (m_ActiveDSP >= m_DSPs.size())
		return;

	if (m_Channel)
	{
		FMOD_RESULT result = m_Channel->addDSP(FMOD_CHANNELCONTROL_DSP_HEAD, m_DSPs[m_ActiveDSP]);
		FMODCheckError(result);

		this->m_ActiveDSP = m_ActiveDSP;
	}
}

void AudioManager::BeginRecording()
{
	if (!m_IsInitialized)
		return;

	if (m_IsRecording)
		return;

	FMOD_RESULT result = m_System->createSound(0, FMOD_LOOP_NORMAL | FMOD_OPENUSER, &m_ExInfo, &m_RecordingSound);
	FMODCheckError(result);

	// https://www.fmod.com/docs/2.00/api/core-api-system.html#system_recordstart
	result = m_System->recordStart(0, m_RecordingSound, true);
	FMODCheckError(result);

	result = m_RecordingSound->getLength(&m_SoundLength, FMOD_TIMEUNIT_PCM);
	FMODCheckError(result);

	m_IsRecording = true;

	m_RecordingLength = 0;
	m_LastRecordPosition = 0;
	m_RecordPosition = 0;
}

void AudioManager::EndRecording()
{
	if (!m_IsInitialized)
		return;

	if (!m_IsRecording)
		return;

	// https://www.fmod.com/docs/2.00/api/core-api-system.html#system_recordstop
	FMOD_RESULT result = m_System->recordStop(0);
	FMODCheckError(result);

	m_IsRecording = false;

	std::ofstream file("recordedSound", std::ios::binary);
	unsigned int dataSize = recordedData.size() * sizeof(float);
	unsigned fileSize = 36 + dataSize;

	float SAMPLE_RATE = 44100;
	file.write("RIFF", 4);
	file.write(reinterpret_cast<const char*>(&fileSize), 4);
	file.write("WAVEfmt ", 8);
	int formatChunkSize = 16; // For PCM
	file.write(reinterpret_cast<const char*>(&formatChunkSize), 4);
	short audioFormat = 1; // For PCM
	file.write(reinterpret_cast<const char*>(&audioFormat), 2);
	short channels = 1;
	file.write(reinterpret_cast<const char*>(&channels), 2);
	file.write(reinterpret_cast<const char*>(&SAMPLE_RATE), 4);
	int byteRate = SAMPLE_RATE * channels * sizeof(float);
	file.write(reinterpret_cast<const char*>(&byteRate), 4);
	short bloackAlign = channels * sizeof(float);
	file.write(reinterpret_cast<const char*>(&bloackAlign), 2);
	short bitsPerSample = 32; // float
	file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
	file.write("data", 4);
	file.write(reinterpret_cast<const char*>(&dataSize), 4);

	file.write(reinterpret_cast<const char*>(m_AudioBuffer.data()), dataSize);
	file.close();
}

void AudioManager::PlayRecordedSound()
{
	if (!m_RecordingSound) {
		return;
	}
	FMOD_RESULT result = m_System->playSound(m_RecordingSound, nullptr, false, &m_Channel);
	FMODCheckError(result);

	if (m_Channel)
	{
		result = m_Channel->setMode(FMOD_LOOP_OFF);
		FMODCheckError(result);

		result = m_Channel->setPaused(false);
		FMODCheckError(result);
	}
}

void AudioManager::PlayARandomSound()
{
	FMOD::Sound* randomSound = nullptr;
	GetRandomSound(&randomSound);
	FMOD_RESULT result;

	if (randomSound)
	{
		result = m_System->playSound(randomSound, nullptr, false, &m_Channel);
		FMODCheckError(result);
	}
}

bool AudioManager::IsSoundPlaying() const
{
	if (m_Channel == nullptr)
		return false;

	bool isPlaying = false;

	FMOD_RESULT result = m_Channel->isPlaying(&isPlaying);
	FMODCheckError(result);

	return isPlaying;
}

FMOD_RESULT AudioManager::RecordCallback(FMOD_SOUND* sound, void* data, unsigned int datalen)
{
	float* buffer = (float*)data;
	size_t sampleCount = datalen / 2;
	m_RingBuffer.Write(buffer, sampleCount);

	return FMOD_OK;
}

void AudioManager::ProcessRecording()
{
	// Retrieves the current recording position of the record buffer in PCM samples.
	// https://www.fmod.com/docs/2.00/api/core-api-system.html#system_getrecordposition
	FMOD_RESULT result = m_System->getRecordPosition(0, &m_RecordPosition);
	FMODCheckError(result);

	if (m_RecordPosition != m_LastRecordPosition)
	{
		void* ptr1 = nullptr;
		void* ptr2 = nullptr;
		unsigned int len1 = 0;
		unsigned int len2 = 0;
		int blockLength = 0;

		blockLength = m_RecordPosition - m_LastRecordPosition;
		if (blockLength < 0)
		{
			blockLength += m_SoundLength;
		}

		// Lock a block of memory of our sound to read data
		// https://www.fmod.com/docs/2.00/api/core-api-sound.html#sound_lock
		result = m_RecordingSound->lock(
			m_LastRecordPosition * m_ExInfo.numchannels * 2, 
			blockLength * m_ExInfo.numchannels * 2,
			&ptr1, &ptr2, &len1, &len2);
		FMODCheckError(result);

		// If the data is looping then ptr1 and len1 will be the last section of the information, and 
		// ptr2 and len2 will start at index 0 again. If the first ptr1 and len1 does not exceed the
		// length then ptr2 will be nul and len2 will be 0
		if (ptr1 && len1 != 0)
		{
 			m_RecordingLength += len1;
			// Write the data to our ringbuffer
			// TODO: m_RingBuffer.Write(...)
			m_RingBuffer.Write(static_cast<float*>(ptr1), len1 / sizeof(float));
		}
		if (ptr2 && len2 != 0)
		{
			m_RecordingLength += len2;
			// Write the data to our ringbuffer
			// TODO: m_RingBuffer.Write(...)
			m_RingBuffer.Write(static_cast<float*>(ptr2), len2 / sizeof(float));
		}

		// Unlock the block of memory
		// https://www.fmod.com/docs/2.00/api/core-api-sound.html#sound_unlock
		result = m_RecordingSound->unlock(ptr1, ptr2, len1, len2);
		FMODCheckError(result);
	}   

	printf("Record buffer pos = %6d : Record time = %02d:%02d.%02d\r", 
		m_RecordPosition, m_RecordingLength / m_ExInfo.defaultfrequency / m_ExInfo.numchannels / 2 / 60,	// Minutes
		(m_RecordingLength / m_ExInfo.defaultfrequency / m_ExInfo.numchannels / 2) % 60,					// Seconds
		(m_RecordingLength / (m_ExInfo.defaultfrequency / 100) / m_ExInfo.numchannels / 2) % 100);			// Milliseconds

	m_LastRecordPosition = m_RecordPosition;
}


void AudioManager::GetRandomSound(FMOD::Sound** sound)
{
	if (!sound || m_DSPs.empty()) {
		return;
	}

	const char* soundFiles[] = {
		"../audio/Forest_Lullabye-AsherFulero.mp3",
		"../audio/spring-singing-of-birds-and-nightingales-177311.mp3",
		"../audio/raven-call-72946.mp3",
		"../audio/zplors-75447.mp3"
	};

	// Randomly selecting sound from above files
	int soundIndex = rand() % std::size(soundFiles);

	FMOD_RESULT result = m_System->createSound(soundFiles[soundIndex], FMOD_DEFAULT, nullptr, sound);
	FMODCheckError(result);
}