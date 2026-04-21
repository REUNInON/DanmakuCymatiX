#include "StochasticEngine.h"
#include <cmath>

StochasticEngine::StochasticEngine()
{
}

void StochasticEngine::Initialize(unsigned int seed)
{
	m_rng.seed(seed);
}

StochasticPayload StochasticEngine::ProcessAudioFrame(float muX, float muY, float poissonMultiplier, float gaussMultiplier, const std::array<float, SonicCore::SPECTRUM_SIZE>& spectrum)
{
	StochasticPayload payload = {};

	// TODO: Map everything!!

	payload.chaosFactor = CalculateEntropy(spectrum);


	//unsigned int audioSeed = static_cast<unsigned int>(payload.chaosFactor * 13371337.0f) ^ static_cast<unsigned int>(poissonMultiplier * 1000.0f);
	//m_rng.seed(audioSeed);

	payload.spawnCount = CalculatePoisson(poissonMultiplier);

	payload.spatialSpread = CalculateSpatialSpread(payload.chaosFactor, 0.05f, gaussMultiplier);
	CalculateBivariateGaussian(muX, muY, payload.spatialSpread, payload.originX, payload.originY);

	return payload;
}

int StochasticEngine::CalculatePoisson(float lambda)
{
	if (lambda < 0.00001f) return 0;

	std::poisson_distribution<int> distribution(lambda);
	// TODO: You can use FFT output as seed instead of m_rng.
	return distribution(m_rng);
}

float StochasticEngine::CalculateEntropy(const std::array<float, SonicCore::SPECTRUM_SIZE>& spectrum)
{
	// 0. Calculate total energy
	float totalEnergy = 0.0f;
	for (float energy : spectrum)
	{
		totalEnergy += energy;
	}

	if (totalEnergy < 0.00001f) return 0.0f; // Avoid division by zero and log of zero

	// 2. Calculate Shannon entropy: H = -sum(p * log2(p))

	float invTotalEnergy = 1.0f / totalEnergy;

	float entropy = 0.0f;

	for (float val : spectrum)
	{
		float p = val * invTotalEnergy;

		if (p > 0.00001f)
		{
			entropy -= p * std::log2(p); // Add small epsilon to avoid log(0)
		}
	}

	return entropy / 9.0f;
}

float StochasticEngine::CalculateSpatialSpread(float energy, float minSpread, float maxSpread)
{
	// Energy surge protection
	if (energy < 0.0f) energy = 0.0f;
	if (energy > 1.0f) energy = 1.0f;

	// LERP
	return minSpread + (maxSpread - minSpread) * energy;
}

void StochasticEngine::CalculateBivariateGaussian(float muX, float muY, float sigma, float& outX, float& outY)
{
	float safeSigma = (std::max)(0.0001f, sigma);

	std::normal_distribution<float> distX(muX, safeSigma);
	std::normal_distribution<float> distY(muY, safeSigma);

	outX = distX(m_rng);
	outY = distY(m_rng);
}
