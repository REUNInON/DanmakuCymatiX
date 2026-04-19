// ===============================================================
// DATA STRUCTS
// ===============================================================

cbuffer GlobalConstants : register(b0)
{
    int spawnCount;
    int stateID;
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
    
    // 2. Skip if the bullet is dead
    if (bullet.state == 0)
        return;
    
    // MVP: BASIC PHYSICS FOR NOW (d = v * t)
    bullet.posX += bullet.velX * deltaTime;
    bullet.posY += bullet.velY * deltaTime;

    // MVP: BASIC STOCHASTIC INFLUENCE (CHAOS FACTOR)
    bullet.posX += sin(totalTime * 10.0 + index) * chaosFactor * deltaTime * 50.0;
    bullet.posY += cos(totalTime * 10.0 + index) * chaosFactor * deltaTime * 50.0;
    
    // 5. Write the updated bullet back to the VRAM
    Bullets[index] = bullet;
}