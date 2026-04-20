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
        bullet.spawnTime = totalTime;

        float hoverX = sin(totalTime * 1.5f) * 0.1f;
        float hoverY = cos(totalTime * 2.1f) * 0.02f;
        float musicJitterX = sin(totalTime * 30.0f) * (chaosFactor * 0.015f);
        
        bullet.posX = originX + hoverX + musicJitterX;
        bullet.posY = 0.85f + hoverY;

        float angle = -1.5708f;
        float speed = 1.4f + (float(index % 5) * 0.01f);

        
        if (stateID == 0)
        {
            // STATE 0
            float streamCount = 15.0f;
            float streamID = float(index % int(streamCount));
            float angleOffset = ((streamID / streamCount) - 0.5f) * 1.6f;
            angle += angleOffset + sin(totalTime * 1.0f) * 0.3f;
            
            bullet.baseRadius = 0.05f;
        }
        else if (stateID == 1)
        {
            // STATE 1: (Crossing Streams)
            float streamCount = 8.0f;
            float streamID = float(index % int(streamCount));
            
            float direction = (index % 2 == 0) ? 1.0f : -1.0f;
            
            angle += ((streamID / streamCount) - 0.5f) * 2.5f + (sin(totalTime * 3.0f) * 0.5f * direction);
            speed *= 0.001f;
            bullet.baseRadius = 0.1f;
        }
        else
        {
            // STATE 2: KAOTiK DROP (Death Blossom )
            angle += float(index) * 2.39996f + (totalTime * 4.0f);
            
            speed = 0.5f + (chaosFactor * 0.08f);
            bullet.baseRadius = 0.1f;
        }

        bullet.velX = cos(angle) * speed;
        bullet.velY = sin(angle) * speed;
        
        //bullet.baseRadius = 0.05f;
        
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
    bullet.posX += cos(totalTime * 10.0 + index) * chaosFactor * deltaTime * 0.2;
    bullet.posY += sin(totalTime * 10.0 + index) * chaosFactor * deltaTime * 0.2;
    
    if (abs(bullet.posX) > 2.0f || abs(bullet.posY) > 2.0f)
    {
        bullet.state = 0;
    }
    
    // 5. Write the updated bullet back to the VRAM
    Bullets[index] = bullet;
}