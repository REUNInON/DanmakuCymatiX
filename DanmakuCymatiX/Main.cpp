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

    // Allocate console
    
    AllocConsole();

    FILE* stream; freopen_s(&stream, "CONOUT$", "w", stdout);

    // 1. OPEN WINDOW
    const UINT WIDTH = 1600;
    const UINT HEIGHT = 1600;
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

    /*
    float screenWidth;
    float screenHeight;

    uint32_t packedStateAndSpawn;
    uint32_t spawnStartIndex;
    float sweepFactor;
    float chaosFactor;

    float deltaTime;

    float originX;
    float originY;
    float spatialSpread;
    float totalTime;

    float playerPosX;
    float playerPosY;

    float hitRadius;
    float grazeRadius;

    float band1;
    float band2;
    float band3;
    */
    
    // 5. GAME LOOP

    auto prevTime = std::chrono::high_resolution_clock::now();
    float accumulator = 0.0f;

	float totalTime = 0.0f;

	// DEFINE GPU CONSTANTS STRUCTURE
	GlobalConstants constants = {};

    constants.screenWidth = (float)WIDTH;
    constants.screenHeight = (float)HEIGHT;

	constants.originX = 0.0f;
	constants.originY = 0.75f;
    constants.hitRadius = 0.02f;
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

				// Maybe remove subBass and brilliance from total energy to focus on midrange for patterns?
				float totalEnergy = bassEnergy + lowMidEnergy + midEnergy + highMidEnergy + presenceEnergy;

#pragma region AUDIO DEBUG

                static int frameCounter = 0;
                frameCounter++;

                if (frameCounter % 20 == 0) {
                    std::cout << "Energy: " << totalEnergy;
                }

#pragma endregion

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

				float bossRoamRadius = 0.0f + (midEnergy * 15.0f);
                
                /*
                if (totalEnergy >= 5.0f) {
                    currentPattern = 4;
                }
                else if (totalEnergy >= 3.5f) {
                    currentPattern = 3;
                }
                else if (totalEnergy >= 1.0f) {
                    currentPattern = 2;
                }
                else {
                    currentPattern = 1;
                }
                */

#pragma region SPECTRAL FLUX
                // 1. SPECTRAL FLUX

                /*
                static float prevBassEnergy = 0.0f;
                float bassHit = (std::max)(0.0f, bassEnergy - prevBassEnergy);
                prevBassEnergy = bassEnergy;

                float rhythmDensity = midEnergy + lowMidEnergy;
                */
                static float prevTotalEnergy = 0.0f;

                float hitTrigger = (std::max)(0.0f, totalEnergy - prevTotalEnergy);
                prevTotalEnergy = totalEnergy;
#pragma endregion

#pragma region DYNAMIC GEAR SYSTEM
				// EXPONENTIAL MOVING AVERAGE FOR SMOOTHING ENERGY VALUES (ADAPTIVE GEAR SYSTEM)
				static float movingAverageEnergy = 0.0f;
				float adaptSpeed = fixedDeltaTime * 5.0f; // ADJUST

                if (totalTime < 1.0f)
                {
                    movingAverageEnergy = totalEnergy;
                }
                else
                {
                    movingAverageEnergy = (adaptSpeed * totalEnergy) + ((1.0f - adaptSpeed) * movingAverageEnergy);
                }

                // RELATIVE ENERGY RATIO
				float energyRatio = totalEnergy / (movingAverageEnergy + 0.0001f);


				// Ratio Smoothing - Hysteresis for stable pattern transitions
                static float smoothedRatio = 5.0f;
                float ratioSmoothSpeed = fixedDeltaTime * 10.0f; 
                smoothedRatio = (ratioSmoothSpeed * energyRatio) + ((1.0f - ratioSmoothSpeed) * smoothedRatio);

                static int currentPattern = 1;

                if (currentPattern < 4 && (smoothedRatio > 1.15f || hitTrigger > 0.15f)) {
                    currentPattern = 4;
                }
                else if (currentPattern == 4 && smoothedRatio < 1.08f) {
                    currentPattern = 3;
                }

                else if (currentPattern < 3 && smoothedRatio > 1.06f) {
                    currentPattern = 3;
                }
                else if (currentPattern == 3 && smoothedRatio < 1.02f) {
                    currentPattern = 2;
                }

                else if (currentPattern < 2 && smoothedRatio > 0.95f) {
                    currentPattern = 2;
                }
                else if (currentPattern == 2 && smoothedRatio < 0.88f) {
                    currentPattern = 1;
                }

				if (frameCounter % 20 == 0)
				std::cout << " || Current Pattern: " << currentPattern << "\n";

#pragma endregion


				float dynamicSpawnRate = hitTrigger * 45.0f;

                StochasticPayload payload = g_stochastic.ProcessAudioFrame
                (
                    0.0f, 0.7f,
                    /*dynamicSpawnRate*/ 0.0f, bossRoamRadius,
                    g_sonicCore.GetRawSpectrum()
				);

                //float dynamicSpawnRate = ((bassHit * 10.0f) + (rhythmDensity * 5.0f)) /** payload.chaosFactor*/;


                payload.spawnCount = g_stochastic.CalculatePoisson(dynamicSpawnRate);

                constants.packedStateAndSpawn = (currentPattern << 16) | (payload.spawnCount & 0xFFFF);
                
                constants.sweepFactor = sweepVelocity;

				constants.chaosFactor = payload.chaosFactor;

                constants.spatialSpread = 1.2f + (midEnergy * 10.0f);

				// POISSON SPAWN COUNT AND RING BUFFER INDEXING
				constants.spawnStartIndex = currentSpawnIndex;
				currentSpawnIndex = (currentSpawnIndex + payload.spawnCount) % (DanmakuPipeline::MAX_BULLETS - 1); // 99999 is player

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