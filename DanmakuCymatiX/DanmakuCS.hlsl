// ===============================================================
// DATA STRUCTS
// ===============================================================

cbuffer GlobalConstants : register(b0)
{
    int spawnCount;
    uint spawnStartIndex;
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
    
    // Poisson
    uint diff = (index - spawnStartIndex + 100000) % 100000;
    
    bool shouldSpawn = (diff < spawnCount);
    
    if (shouldSpawn)
    {
        bullet.state = 1;
        
        bullet.posX = originX + (sin(totalTime * 1.3f) * 0.5f) + (cos(totalTime * 0.7f) * 0.3f);
                
        bullet.posY = 0.5f + (sin(totalTime * 2.2f) * 0.05f);
        
        float streamCount = 15.0f;
        float streamID = float(index % int(streamCount));
        float angleOffset = ((streamID / streamCount) - 0.5f) * 1.5f; // varying spread
        float angle = -1.5708f + angleOffset + sin(totalTime * 1.0f) * 1.0f;
        
        
        float speed = 1.4f + (float(index % 5) * 0.01f);
        
        if (stateID == 0)
        {
            // STATE 0: NORMAL
            float streamCount = 15.0f;
            float streamID = float(index % int(streamCount));
            float angleOffset = ((streamID / streamCount) - 0.5f) * 1.5f; // varying spread
            angle += /*angleOffset + */sin(totalTime * 1.0f) * 1.0f;
        }
        
        bullet.velX = cos(angle) * speed;
        bullet.velY = sin(angle) * speed;
        bullet.baseRadius = 0.1f;
        
        Bullets[index] = bullet;
        return;
    }
    
    // 2. Skip if the bullet is dead (CHANGED FOR THE MVP)
    if (bullet.state == 0)
        return;
    
    // MVP: BASIC PHYSICS FOR NOW (d = v * t)
    bullet.posX += bullet.velX * deltaTime;
    bullet.posY += bullet.velY * deltaTime;

    // MVP: BASIC STOCHASTIC INFLUENCE (CHAOS FACTOR)
    bullet.posX += cos(totalTime * 10.0 + index) * chaosFactor * deltaTime * 1.5;
    bullet.posY += sin(totalTime * 10.0 + index) * chaosFactor * deltaTime * 1.5;
    
    if (abs(bullet.posX) > 2.0f || abs(bullet.posY) > 2.0f)
    {
        bullet.state = 0;
    }
    
    // 5. Write the updated bullet back to the VRAM
    Bullets[index] = bullet;
}