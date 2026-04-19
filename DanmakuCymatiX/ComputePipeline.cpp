#include "ComputePipeline.h"

#include "DanmakuCS.h"

// Helper macro for checking HRESULTs one by one.
// Usage: DX(your_dx_call());
#ifndef DX
#define DX(call) do { HRESULT _hr = (call); if (FAILED(_hr)) return _hr; } while(0)
#endif

HRESULT ComputePipeline::Initialize(Renderer& renderer)
{
    return E_NOTIMPL;
}

void ComputePipeline::Shutdown()
{
}

void ComputePipeline::Dispatch(Renderer& renderer, const GlobalConstants& constants)
{
}

/// <summary>
/// Creates the root signature for the compute shader pipeline.
/// </summary>
/// <param name="renderer"></param>
/// <returns></returns>
HRESULT ComputePipeline::CreateRootSignature_(Renderer& renderer)
{
	D3D12_ROOT_PARAMETER rootParameters[2] = {};

	// ======================================================================
	// SLOT 0: GLOBAL CONSTANTS (b0) - Sonic Core Data + Stochastic Payload
	// ======================================================================

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[0].Constants.ShaderRegister = 0; // b0
	rootParameters[0].Constants.RegisterSpace = 0;
	rootParameters[0].Constants.Num32BitValues = sizeof(GlobalConstants) / 4;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ======================================================================
	// SLOT 1: UAV BUFFER (u0) - Bullet Data Buffer
	// ======================================================================

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParameters[1].Descriptor.ShaderRegister = 0; // u0
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ======================================================================
	// ROOT SIGNATURE DESCRIPTION
	// ======================================================================

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters = _countof(rootParameters);
	desc.pParameters = rootParameters;
	desc.NumStaticSamplers = 0;
	desc.pStaticSamplers = nullptr;

	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
	             D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// ======================================================================
	// SERIALIZE AND CREATE ROOT SIGNATURE
	// ======================================================================

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	DX(hr);

	DX(renderer.GetDevice()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    return S_OK;
}

/// <summary>
/// Creates the compute pipeline state object (PSO) for the compute shader.
/// </summary>
/// <param name="renderer"></param>
/// <returns></returns>
HRESULT ComputePipeline::CreatePSO_(Renderer& renderer)
{
	// =======================================================================
	// PSO DESCRIPTION
	// =======================================================================

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};

	desc.pRootSignature = m_rootSignature.Get();

	desc.CS = { g_DanmakuCS, sizeof(g_DanmakuCS) };

	desc.NodeMask = 0;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	// =======================================================================
	// CREATE PSO
	// =======================================================================

	DX(renderer.GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf())));

    return S_OK;
}

/// <summary>
/// Creates the structured buffer for bullet data. This buffer will be used by the compute shader to read/write bullet information such as position, velocity, and state.
/// </summary>
/// <param name="renderer"></param>
/// <returns></returns>
HRESULT ComputePipeline::CreateBulletBufferAndViews_(Renderer& renderer)
{
	// =======================================================================
	// UAV DATA BUFFER DESCRIPTION
	// =======================================================================

	D3D12_RESOURCE_DESC desc = {};

	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = MAX_BULLETS * sizeof(BulletGPU);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;

	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	// =======================================================================
	// CREATE UAV BUFFER
	// =======================================================================

	DX(renderer.CreateBuffer(desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, m_bulletBuffer));

    return S_OK;
}
