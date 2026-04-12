#pragma once
#include <string>
#include <array>
#include "bass.h"

enum class AudioBand
{
	SubBass,	// Index 0-1: 20-60 Hz
	Bass,		// Index 2-5: 60-250 Hz
	LowerMids,	// Index 6-11: 250-500 Hz
	Midrange,	// Index 12-46: 500-2000 Hz
	HigherMids,	// Index 47-92: 2-4 kHz
	Presence,	// Index 93-139: 4-6 kHz
	Brilliance,	// Index 140-232: 6-20 kHz
	Air			// Index 233-511: 20-22050 Hz
};

class SonicCore
{
public:
	// Fast Fourier Transform (FFT) Size
	static constexpr size_t SPECTRUM_SIZE = 512;

	SonicCore();
	~SonicCore();

	// Control Methods
	bool Initialize(uint32_t sampleRate = 48000);
	bool LoadAudio(const std::string& filePath);
	void Play();
	void Tick();

	const std::array<float, SPECTRUM_SIZE>& GetRawSpectrum() const { return m_spectrum; }
	float GetBandEnergy(AudioBand band) const;

private:
	// Internal State for BASS
	unsigned long m_streamHandle;

	// Cached Spectrum Data
	std::array<float, SPECTRUM_SIZE> m_spectrum;

	// Data Oriented Design: Predefined frequency band indices for quick access
	struct BandRange
	{
		size_t start;
		size_t end;

		float invCount; // Precomputed inverse of (end - start + 1) to avoid division
	};
	// Lookup table for band indices (precomputed based on typical audio spectrum divisions)
	std::array<BandRange, 8> m_bandRanges;


};

