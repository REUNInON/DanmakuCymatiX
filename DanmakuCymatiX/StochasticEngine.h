#pragma once
#include <array>
#include <random>

#include "SonicCore.h"

struct StochasticPayload
{
	int spawnCount; // Number of samples generated from Poisson distribution
	int stateID; // Markov Chain state identifier
	float chaosFactor; // Shannon entropy-based chaos factor (0.0 to 1.0)
	float originX; // Bivariate Gaussian spatial x-coordinate
	float originY; // Bivariate Gaussian spatial y-coordinate
	float spatialSpread; // Bivariate Gaussian spread (standard deviation)
};

// TODO: Stochastic Engine is not fully agnostic of Sonic Core.
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

	// ===============================================================
	// STOCHASTIC CALCULATIONS
	// ===============================================================

	// POISSON DISTRIBUTION
	/// <summary>
	/// Returns number of events based on a Poisson distribution.
	/// </summary>
	/// <param name="lambda">The expected number of events (mean) for the Poisson distribution.</param>
	int CalculatePoisson(float lambda);

	// SHANNON ENTROPHY
	/// <summary>
	/// Calculates the Shannon entropy of the given audio spectrum.
	/// </summary>
	/// <param name="spectrum">Array containing energy of each frequency of the FFT output.</param>
	/// <returns></returns>
	float CalculateEntropy(const std::array<float, SonicCore::SPECTRUM_SIZE>& spectrum);

	// BIVARIATE GAUSS
	float CalculateSpatialSpread(float energy, float minSpread, float maxSpread);

	// MARKOV CHAIN
	int UpdateMarkovState(float transitionTrigger);

private:
	std::mt19937 m_rng; // Mersenne Twister random number generator

	int currentState; // Current state for Markov Chain

	// TODO: Markov Chain Matrix and Transition Probabilities
};

