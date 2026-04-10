#include "SonicCore.h"
#include <iostream>
#include <cassert>

SonicCore::SonicCore() : m_streamHandle(0)
{
	// Initialize spectrum data to zero
	m_Spectrum.fill(0.0f);

	// Precompute Band Ranges
	m_bandRanges[static_cast<int>(AudioBand::SubBass)] = { 0, 1, 1.0f / (1 - 0 + 1) };
	m_bandRanges[static_cast<int>(AudioBand::Bass)] = { 2, 5, 1.0f / (5 - 2 + 1) };
	m_bandRanges[static_cast<int>(AudioBand::LowerMids)] = { 6, 11, 1.0f / (11 - 6 + 1) };
	m_bandRanges[static_cast<int>(AudioBand::Midrange)] = { 12, 46, 1.0f / (46 - 12 + 1) };
	m_bandRanges[static_cast<int>(AudioBand::HigherMids)] = { 47, 92, 1.0f / (92 - 47 + 1) };
	m_bandRanges[static_cast<int>(AudioBand::Presence)] = { 93, 139, 1.0f / (139 - 93 + 1) };
	m_bandRanges[static_cast<int>(AudioBand::Brilliance)] = { 140, 232, 1.0f / (232 - 140 + 1) };
	m_bandRanges[static_cast<int>(AudioBand::Air)] = { 233, 511, 1.0f / (511 - 233 + 1) };
}

SonicCore::~SonicCore()
{
	if (m_streamHandle != 0) // RAII Guard
	{
		BASS_StreamFree(m_streamHandle); // Free the stream
	}

	BASS_Free(); // Free BASS resources
}

bool SonicCore::Initialize(uint32_t sampleRate)
{
	if (!BASS_Init(-1, sampleRate, 0, 0, NULL))
	{
		std::cerr << "[ERROR] BASS initialization failed. Error Code: " << BASS_ErrorGetCode() << "\n";
		return false;
	}

	return true;
}

bool SonicCore::LoadAudio(const std::string& filePath)
{
	// Free RAM of any existing stream
	if (m_streamHandle != 0)
		BASS_StreamFree(m_streamHandle);

	// Decode the audio file into a stream (supports various formats)
	m_streamHandle = BASS_StreamCreateFile(FALSE, filePath.c_str(), 0, 0, BASS_SAMPLE_LOOP);

	if (m_streamHandle == 0)
	{
		std::cerr << "[ERROR] Failed to load audio file: " << filePath << "\n";
		return false;
	}

	return true;
}

void SonicCore::Play()
{
	assert(m_streamHandle != 0 && "Audio stream must be loaded before playing!");
	BASS_ChannelPlay(m_streamHandle, FALSE);
}

void SonicCore::Tick()
{
	assert(m_streamHandle != 0 && "Audio stream must be loaded before analyzing!");
	BASS_ChannelGetData(m_streamHandle, m_Spectrum.data(), BASS_DATA_FFT1024);
}

float SonicCore::GetBandEnergy(AudioBand band) const
{
	int index = static_cast<int>(band);
	const BandRange& range = m_bandRanges[index];

	float sum = 0.0f;
	for (size_t i = range.start; i <= range.end; ++i)
	{
		sum += m_Spectrum[i];
	}

	int count = (range.end - range.start + 1);

	return sum * range.invCount; // avoid division by zero
}
