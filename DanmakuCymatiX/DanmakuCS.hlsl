cbuffer GlobalConstants : register(b0)
{
    float screenWidth;
    float screenHeight;
    
    uint packedStateAndSpawn; // 16 bits for state ID, 16 bits for spawn count (Bit Packing). Spawn Limit: 65535 per Frame
    uint spawnStartIndex; // For the ring buffer of bullets (Poisson distribution)
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
};

struct BulletGPU
{
    float posX;
    float posY;
    float velX;
    float velY;

    float spawnTime;
    uint state;
    float baseRadius;
    float spikes;
};

RWStructuredBuffer<BulletGPU> Bullets : register(u0);
RWStructuredBuffer<uint> HitCounter : register(u1);

// RNG FOR BIVARIATE GAUSS
float hash(uint n)
{
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return float(n & 0x7fffffffU) / 2147483648.0f;
}

// ===============================================================
// DANMAKU PATTERN METHODS
// ===============================================================

// N-Way Spiral: 5 streams rotating over time. Rotation speed scales with Treble (band3).
void PatternNWaySpiral(uint index, float t, float b3, out float angle, out float speed)
{
    float streamCount = 5.0f;
    float streamID = float(index % int(streamCount));
    float baseAngle = (streamID / streamCount) * 6.28318f;
    float spinSpeed = 2.0f + (b3 * 5.0f);
    angle = baseAngle + (t * spinSpeed);
    speed = 1.0f;
}

// The Sweeper: A fan of streams swaying left and right. Sway width scales with Bass (band1).
void PatternSweeper(uint index, float t, float b1, out float angle, out float speed)
{
    float streamCount = 6.0f;
    float streamID = float(index % int(streamCount));
    float fanAngle = -1.5708f - 1.0f + (streamID * 0.4f);
    float swayAmount = 1.0f + (b1 * 2.0f);
    angle = fanAngle + sin(t * 3.0f) * swayAmount;
    speed = 0.5f;
}

// Golden Ratio: Sunflower seed distribution forming a mesmerizing spiral.
void PatternGoldenRatio(uint index, float t, out float angle, out float speed)
{
    float goldenAngle = 2.39996f;
    angle = float(index) * goldenAngle + (t * 1.5f);
    speed = 0.02f + (float(index % 100) * 0.005f);
}

// Audio-Reactive Nova: 360-degree bursts. Burst speed scales heavily with Bass (band1).
void PatternAudioNova(uint index, float b1, out float angle, out float speed)
{
    float bulletsPerRing = 32.0f;
    float ringIndex = float(index % int(bulletsPerRing));
    angle = (ringIndex / bulletsPerRing) * 6.28318f;
    speed = 0.1f + (b1 * 0.75f);
}

// Classic Shotgun: Pure Box-Muller Gaussian spread downwards.
void PatternClassicShotgun(float spread, float b2, float z0, float z1, out float angle, out float speed)
{
    float baseAngle = -1.5708f;
    angle = baseAngle + (z0 * spread);
    speed = 0.5f + (z1 * 0.02f) + (b2 * 2.5f);
}


// ===============================================================
// COMPUTE KERNEL
// ===============================================================

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint index = DTid.x;
    
    if (index >= 100000 /* TODO: Fix this magic number */)
        return;

    // 1. Read the bullet from the VRAM
    BulletGPU bullet = Bullets[index];
    
    // ===============================================================
    // PLAYER MOVEMENT SCOPE-FRIENDLY HACK
    // ===============================================================
    if (index == 99999)
    {
        bullet.state = 1; // UNDYING
        
        bullet.posX = playerPosX;
        bullet.posY = playerPosY;
        
        // NO FORCE APPLIED, PLAYER IS CONTROLLED EXTERNALLY
        bullet.velX = 0.0f;
        bullet.velY = 0.0f;
        
        // SIZE
        bullet.baseRadius = 0.6f;
        
        Bullets[index] = bullet;
        return;
    }
    
    uint spawnCount = packedStateAndSpawn & 0xFFFF; // Lower 16 bits for spawnCount
    uint stateID = (packedStateAndSpawn >> 16); // Upper 16 bits for stateID
    
    // Poisson
    uint diff = (index - spawnStartIndex + 100000) % 100000;
    bool shouldSpawn = (diff < spawnCount);
        
    if (shouldSpawn)
    {
        bullet.state = 1;
        bullet.spawnTime = totalTime;
        
        // Origin logic
        bullet.posX = originX;
        bullet.posY = originY;
        
        uint audioSeed = (uint) (chaosFactor * 10000.0f) + (uint) (band1 * 1000.0f);
        
        // Random variables for Gaussian/Shotgun patterns
        float u1 = max(0.00001f, hash(index + (uint) (totalTime * 1000.0f) + audioSeed));
        float u2 = hash(index + 1337U + (uint) (totalTime * 1000.0f) + audioSeed);
        float z0 = sqrt(-2.0f * log(u1)) * cos(6.28318f * u2);
        float z1 = sqrt(-2.0f * log(u1)) * sin(6.28318f * u2);

        float angle = 0.0f;
        float speed = 0.0f;
        
        int pattern = stateID;

        if (pattern == 0)
        {
            PatternNWaySpiral(index, totalTime, band3, angle, speed);
        }
        else if (pattern == 1)
        {
            PatternSweeper(index, totalTime, band1, angle, speed);
        }
        else if (pattern == 2)
        {
            PatternGoldenRatio(index, totalTime, angle, speed);
        }
        else if (pattern == 3)
        {
            PatternAudioNova(index, band1, angle, speed);
        }
        else
        {
            PatternClassicShotgun(spatialSpread, band2, z0, z1, angle, speed);
        }

        bullet.velX = cos(angle) * speed;
        bullet.velY = sin(angle) * speed;
        
        
        // DENSITY BASED RADIUS
        
        float densityRatio = saturate(abs(band1 * 5.0f));
        float spawnRadius = lerp(0.4f, 0.05f, densityRatio);
        bullet.baseRadius = spawnRadius;
        
        bullet.spikes = spawnRadius;
        
        Bullets[index] = bullet;
        return;
    }
    // 2. Skip if the bullet is dead
    if (bullet.state == 0)
        return;
    
    // ===============================================================
    // HYBRID PHYSICS: BULLET HELL CORE + AUDIO REACTIVE 
    // ===============================================================
    
    // Bullet Hell Core
    float currentAngle = atan2(bullet.velY, bullet.velX);
    float currentSpeed = length(float2(bullet.velX, bullet.velY));
    
    // A. BASS PUMP ACCELERATOR
    // TODO: Might be bad for the game feel.
    float flowSpeed = 2.0f + (pow(band1, 3.0f) * 10.0f);
    
    // B. SWEEP VORTEX
    float turnRate = sweepFactor * 1.0f;
    
    // Jitter
    float jitter = sin(totalTime * 15.0f + index) * band3 * 0.5f;
    
    // C. UPDATE ORBIT
    currentAngle += (turnRate + jitter) * deltaTime;
    
    bullet.velX = cos(currentAngle) * currentSpeed;
    bullet.velY = sin(currentAngle) * currentSpeed;

    // D. APPLY PHYSICS
    bullet.posX += bullet.velX * flowSpeed * deltaTime;
    bullet.posY += bullet.velY * flowSpeed * deltaTime;

    // E. PULSATING RADIUS
    //bullet.baseRadius = bullet.spikes + (pow(band1, 2.0f) * 0.02f);

    // ===============================================================   
    // COLLISION
    // ===============================================================
    
    float dx = bullet.posX - playerPosX;
    float dy = bullet.posY - playerPosY;
    float distSq = (dx * dx) + (dy * dy);
    
    float totalRadius = hitRadius;
    
    if (distSq < (totalRadius * totalRadius))
    {
        bullet.baseRadius = 10.0f; // TODO: Other visual feedback.
        InterlockedAdd(HitCounter[0], 1);
        bullet.state = 0;
    }
    else if (distSq < ((bullet.baseRadius + grazeRadius) * (bullet.baseRadius + grazeRadius)))
    {
        // GRAZE
    }
    
    // 4. Kill boundary
    if (abs(bullet.posX) > 2.0f || abs(bullet.posY) > 2.0f)
    {
        bullet.state = 0;
    }
    
    // 5. Write the updated bullet back to the VRAM
    Bullets[index] = bullet;
}