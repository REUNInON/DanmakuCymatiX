#pragma once
#include "Renderer.h"

class BulletPipeline
{
	ID3D12PipelineState CreatePSO(Renderer renderer);

	ID3D12RootSignature CreateRS(Renderer renderer);

	HRESULT Initialize(Renderer renderer);
};

