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

#pragma region WINDOW SETUP

    // 0. UNUSED PARAMETERS
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    FILE* stream; freopen_s(&stream, "CONOUT$", "w", stdout);

    // 1. OPEN WINDOW
    const UINT WIDTH = 1920;
    const UINT HEIGHT = 1080;
    HWND hWnd = SetupWindow(hInstance, WIDTH, HEIGHT);

    if (!hWnd) return -1;

    MSG msg = {};

#pragma endregion
#pragma region INITIALIZATION

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

#pragma endregion
#pragma region GAME LOOP
#pragma region VARIABLES
    
    
    // 5. GAME LOOP

    auto prevTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;

	float totalTime = 0.0f;

	// DEFINE GPU CONSTANTS STRUCTURE
	GlobalConstants constants = {};

    constants.screenWidth = (float)WIDTH;
    constants.screenHeight = (float)HEIGHT;

	constants.originX = 0.0f;
	constants.originY = 0.0f;
    constants.hitRadius = 0.045f;
	constants.grazeRadius = 0.2f;

    uint32_t currentSpawnIndex = 0; // For ring buffer of bullets (Poisson)
#pragma endregion

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {

#pragma region SIMULATION
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

#pragma endregion
#pragma region PLAYER CONTROLLER
			// PLAYER SPEED CONTROL
            float playerSpeed = 0.75f;

            // HOLD SHIFT TO MOVE SLOWER
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) playerSpeed *= 0.5f;

            if (GetAsyncKeyState('D') & 0x8000) constants.playerPosX += playerSpeed * fixedDeltaTime;
            if (GetAsyncKeyState('A') & 0x8000) constants.playerPosX -= playerSpeed * fixedDeltaTime;
            if (GetAsyncKeyState('W') & 0x8000) constants.playerPosY += playerSpeed * fixedDeltaTime;
            if (GetAsyncKeyState('S') & 0x8000) constants.playerPosY -= playerSpeed * fixedDeltaTime;

			if (GetAsyncKeyState(VK_RIGHT) & 0x8000) constants.playerPosX += playerSpeed * fixedDeltaTime;
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) constants.playerPosX -= playerSpeed * fixedDeltaTime;
            if (GetAsyncKeyState(VK_UP) & 0x8000) constants.playerPosY += playerSpeed * fixedDeltaTime;
            if (GetAsyncKeyState(VK_DOWN) & 0x8000) constants.playerPosY -= playerSpeed * fixedDeltaTime;

            constants.playerPosX = std::clamp(constants.playerPosX, -2.0f, 2.0f);
            constants.playerPosY = std::clamp(constants.playerPosY, -2.0f, 2.0f);

#pragma endregion

            while (accumulator >= fixedDeltaTime)
            {

#pragma region AUDIO ANALYSIS

                g_sonicCore.Tick();

				// Get all band energies for use in patterns and spawn rates

				float subBassEnergy = g_sonicCore.GetBandEnergy(AudioBand::SubBass);
				float bassEnergy = g_sonicCore.GetBandEnergy(AudioBand::Bass);
				float lowMidEnergy = g_sonicCore.GetBandEnergy(AudioBand::LowerMids);
				float midEnergy = g_sonicCore.GetBandEnergy(AudioBand::Midrange);
				float highMidEnergy = g_sonicCore.GetBandEnergy(AudioBand::HigherMids);
				float presenceEnergy = g_sonicCore.GetBandEnergy(AudioBand::Presence);
				float brillianceEnergy = g_sonicCore.GetBandEnergy(AudioBand::Brilliance);

				constants.band1 = bassEnergy;
				constants.band2 = lowMidEnergy;
				constants.band3 = highMidEnergy;

#pragma endregion

#pragma region PATTERN LOGIC
#pragma endregion
#pragma region BOSS BEHAVIOR
#pragma endregion
#pragma region SPAWN LOGIC
#pragma endregion
#pragma region EFFECT CONTROLS
#pragma endregion
                // SWEEP
                float currentTilt = highMidEnergy - bassEnergy;
                static float previousTilt = 0.0f;
                float sweepVelocity = (currentTilt - previousTilt) / fixedDeltaTime;
                previousTilt = currentTilt;
                constants.sweepFactor = sweepVelocity;

				//float dynamicSpawnRate = bassEnergy *  20.0f;
				//float dynamicSpawnRate = 0.0f + (band3Energy * 25.0f); // Base rate + scaled by band energy

				float bossRoamRadius = 0.2f + (midEnergy * 500.0f);

                static float patternCooldown = 0.0f;
                static int currentPattern = 4;
                patternCooldown -= fixedDeltaTime;

                if (bassEnergy > 0.15f && patternCooldown <= 0.0f)
                {
                    currentPattern = (currentPattern + 1) % 5;
                    patternCooldown = 2.0f;
                }

                // 1. SPECTRAL FLUX
                static float prevBassEnergy = 0.0f;
                float bassHit = (std::max)(0.0f, bassEnergy - prevBassEnergy);
                prevBassEnergy = bassEnergy;

                float rhythmDensity = midEnergy + lowMidEnergy;

                StochasticPayload payload = g_stochastic.ProcessAudioFrame
                (
                    0.0f, 0.7f,
                    /*dynamicSpawnRate*/ 0.0f, bossRoamRadius,
                    g_sonicCore.GetRawSpectrum()
				);

                float dynamicSpawnRate = ((bassHit * 150.0f) + (rhythmDensity * 20.0f)) * payload.chaosFactor;

                dynamicSpawnRate = std::clamp(dynamicSpawnRate, 0.0f, 100.0f);

                payload.spawnCount = g_stochastic.CalculatePoisson(dynamicSpawnRate);

                constants.packedStateAndSpawn = (currentPattern << 16) | (payload.spawnCount & 0xFFFF);
				float sweepFactor = sweepVelocity * 1.5f;
                constants.sweepFactor = sweepFactor;

				constants.chaosFactor = payload.chaosFactor;

                constants.spatialSpread = 1.2f + (midEnergy * 10.0f);

				// BOSS MOVEMENT CALCULATION
                float bossSpeed = 1.5f + (highMidEnergy * 25.0f);
                
                constants.originX += sweepFactor * bossSpeed * fixedDeltaTime;

                // Boss stays on the left side fix:
                constants.originX = std::lerp(constants.originX, 0.0f, 2.0f * fixedDeltaTime);
                constants.originX = std::clamp(constants.originX, -1.5f, 1.5f);

                constants.originY = 0.8f - (bassEnergy * 0.1f);

				// OLDER GAUSSIAN ROAMING
                // constants.originX += (payload.originX - constants.originX) * bossSpeed * fixedDeltaTime;
                // constants.originY += (payload.originY - constants.originY) * bossSpeed * fixedDeltaTime;

                constants.originX = std::clamp(constants.originX, -1.5f, 1.5f);

				// POISSON SPAWN COUNT AND RING BUFFER INDEXING
				constants.spawnStartIndex = currentSpawnIndex;
				currentSpawnIndex = (currentSpawnIndex + payload.spawnCount) % DanmakuPipeline::MAX_BULLETS;

                accumulator -= fixedDeltaTime;
            }

#pragma region RENDERING
            // ===========================================================================
            // RENDERING CODE
            // ===========================================================================

			constants.deltaTime = frameTime;
			constants.totalTime = totalTime;

            g_renderer.BeginFrame();
            g_pipeline.Dispatch(g_renderer, constants);
			g_renderer.IssueBarrier(g_renderer.GetCommandList(), g_pipeline.GetBulletBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			g_pipeline.Render(g_renderer, constants);
			g_renderer.IssueBarrier(g_renderer.GetCommandList(), g_pipeline.GetBulletBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            g_renderer.EndFrame();
#pragma endregion

        }
    }
#pragma endregion

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

    HWND hWnd = CreateWindow(CLASS_NAME, L"Danmaku CymatiX",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    return hWnd;
}