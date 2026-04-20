#include <iostream>
#include "bass.h"
#include "Renderer.h"
#include "SonicCore.h"
#include "StochasticEngine.h"
#include "DanmakuPipeline.h"
#include <windows.h>
#include <algorithm>

#include <chrono>

// GLOBAL VARIABLES
Renderer g_renderer;
SonicCore g_sonicCore;
StochasticEngine g_stochastic;
DanmakuPipeline g_pipeline;



// Window Creation Helper
HWND SetupWindow(HINSTANCE hInstance, int width, int height);

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{

    // 0. UNUSED PARAMETERS
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    FILE* stream; freopen_s(&stream, "CONOUT$", "w", stdout);

    // 1. OPEN WINDOW
    const UINT WIDTH = 1080;
    const UINT HEIGHT = 720;
    HWND hWnd = SetupWindow(hInstance, WIDTH, HEIGHT);

    if (!hWnd) return -1;

	// 2. INITIALIZE SONIC CORE

    if (!g_sonicCore.Initialize())
    {
        MessageBox(hWnd, L"Failed to initialize Sonic Core!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
	}

    if (!g_sonicCore.LoadAudio("audio.mp3"))
    {
        MessageBox(hWnd, L"Failed to load audio file!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
	}

	g_sonicCore.Play(); // START THE JAM!


    // 3. INITIALIZE RENDERER
    if (FAILED(g_renderer.Initialize(hWnd, WIDTH, HEIGHT)))
    {
        MessageBox(hWnd, L"Failed to initialize Renderer!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 4. INITIALIZE DANMAKU PIPELINE
    if (FAILED(g_pipeline.Initialize(g_renderer)))
    {
        MessageBox(hWnd, L"Failed to initialize Danmaku Pipeline!", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

	g_stochastic.Initialize(1337);

    // 5. GAME LOOP

    MSG msg = {};

    auto prevTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;
	float totalTime = 0.0f; // For sin/cos waves in bullet patterns

	// DEFINE GPU CONSTANTS STRUCTURE
	GlobalConstants constants = {};

	constants.originX = 0.0f;
	constants.originY = 0.0f;
    constants.hitRadius = 5.0f;
	constants.grazeRadius = 15.0f;

    uint32_t currentSpawnIndex = 0; // For ring buffer of bullets (Poisson)

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // ===========================================================================
            // FIXED TIMESTEP SIMULATION
            // ===========================================================================
            auto currentTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float>(currentTime - prevTime).count();
            prevTime = currentTime; if (frameTime > 0.25f) frameTime = 0.25f; // Avoid deadly long frames

            accumulator += frameTime;
			totalTime += frameTime;

            // ===========================================================================
            // SIMULATION STEP
            // ============================================================================
            const float fixedDeltaTime = 0.016f; // 60 FPS

            while (accumulator >= fixedDeltaTime)
            {
				g_sonicCore.Tick();

				float bassEnergy = g_sonicCore.GetBandEnergy(AudioBand::SubBass);

				float trebleEnergy = g_sonicCore.GetBandEnergy(AudioBand::Presence);


				float dynamicSpawnRate = 0.0f + (bassEnergy * 50.0f); // Base rate + scaled by bass energy

                StochasticPayload payload = g_stochastic.ProcessAudioFrame
                (
                    0, 0,
                    dynamicSpawnRate, 1.0f,
                    g_sonicCore.GetRawSpectrum()
				);

				constants.chaosFactor = payload.chaosFactor + (trebleEnergy * 15.0f);

				// POISSON SPAWN COUNT AND RING BUFFER INDEXING
				constants.spawnCount = payload.spawnCount;
				constants.spawnStartIndex = currentSpawnIndex;
				currentSpawnIndex = (currentSpawnIndex + payload.spawnCount) % DanmakuPipeline::MAX_BULLETS;

                accumulator -= fixedDeltaTime;
            }

            // ===========================================================================
            // RENDERING CODE
            // ===========================================================================

			constants.deltaTime = frameTime;
			constants.totalTime = totalTime;


            g_renderer.BeginFrame();
            g_pipeline.Dispatch(g_renderer, constants);
			g_renderer.IssueBarrier(g_renderer.GetCommandList(), g_pipeline.GetBulletBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			g_pipeline.Render(g_renderer);
			g_renderer.IssueBarrier(g_renderer.GetCommandList(), g_pipeline.GetBulletBuffer(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            g_renderer.EndFrame();
        }
    }

    g_renderer.Shutdown();

    return (int)msg.wParam;
}

// ============================================================================
// WINDOW CREATION HELPER (Standard Windows)
// ============================================================================
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND SetupWindow(HINSTANCE hInstance, int width, int height)
{
    const wchar_t* CLASS_NAME = L"SimulationWindow";
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassEx(&wc);

    RECT rc = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindow(CLASS_NAME, L"Squish Engine",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    return hWnd;
}