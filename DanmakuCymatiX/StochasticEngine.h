#pragma once
#include <array>
#include <random>

#include "SonicCore.h"

struct StochasticPayload
{
	int spawnCount; // Number of samples generated from Poisson distribution
	int stateID; // Markov Chain state identifier
	float chaosFactor; // Shannon entropy-based chaos factor (0.0 to 1.0)
	float originX; float originY; // Bivariate Gaussian spatial coordinates
	float spatialSpread; // Bivariate Gaussian spread (standard deviation)
};

class StochasticEngine
{
public:
	StochasticEngine();
	~StochasticEngine() = default;

	void Initialize(unsigned int seed);

	StochasticPayload ProcessAudioFrame
	(
		float muX, float muY, // Target spatial coordinates

		float poissonMultiplier,
		float gaussMultiplier,

		const std::array<float, SonicCore::SPECTRUM_SIZE>& spectrum
	);

private:
	std::mt19937 m_rng; // Mersenne Twister random number generator

	int currentState; // Current state for Markov Chain

	// TODO: Markov Chain Matrix and Transition Probabilities
};

