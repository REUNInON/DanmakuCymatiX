# DanmakuCymatiX

## Intro
Danmaku CymatiX is an audio driven bullet hell game which analyses audio in real-time to generate danmaku patterns. The project is written in *C++* with *DirectX 12* graphics API. It uses *BASS Audio Library* by Un4seen Developments (https://www.un4seen.com) to analyze audio, implementing its Fast Fourrier Transform to divide audio into 512 different frequencies each frame.  

## Pipeline
### Load Audio & FFT
Uses BASS[(https://www.un4seen.com/)].

### Stochastic Engine
1. Poisson Distribution
2. Markov Chain State Machine
3. Shannon Entrophy Calculation
4. Bivariate Gaussian - Position Calculation

## The Sonic Core
The Sonic Core system uses BASS Audio Library to play the music and to analyze it every frame. Even though the function of BASS returns amplitudes of 512 different frequencies, the system takes the average of frequency ranges and only operates on the following audio bands:

```
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
```

These 8 different audio bands are available for other methods to use. The band energies from the previous frame can also be used to calculate the energy difference, which makes the changes more noticable. The square root of the energies is used, that way the energies impact the things they impact more and more like huiman hearing.

## The Stochastic Engine
The Stochastic Engine serves a payload containing spawn count, bivariate gaussian and a chaos factor which are recalculated at the beginning of every frame. Every frame, the number of bullets fired are determined by *poisson distribution*. *Shannon entrophy* is used to calculate the chaos factor of the total audio energy, which is can be used for different visual FX and gameplay elements. The spawner (enemy) position can be moved each frame using the *bivariate gaussian* numbers but it makes the enemy movement unpredictable and might effect the immersion negatively.

## Renderer & Shaders
The project does not use a commercial game engine like Unity and UE5, it uses it's own propreitary engine written in C++ using *WINAPI* for the window management, main game loop and player controls, DirectX 12 API for the renderer and GPU access.
