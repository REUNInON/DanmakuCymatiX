#include "StochasticEngine.h"

StochasticEngine::StochasticEngine()
{
}

void StochasticEngine::Initialize(unsigned int seed)
{
}

StochasticPayload StochasticEngine::ProcessAudioFrame(float muX, float muY, float poissonMultiplier, float gaussMultiplier, const std::array<float, SonicCore::SPECTRUM_SIZE>& spectrum)
{
	return StochasticPayload();
}
