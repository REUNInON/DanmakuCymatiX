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

// RNG FOR BIVARIATE GAUSS
float hash(uint n)
{
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return float(n & 0x7fffffffU) / 2147483648.0f;
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
    
    // Poisson
    uint diff = (index - spawnStartIndex + 100000) % 100000;
    
    bool shouldSpawn = (diff < spawnCount);
        
    if (shouldSpawn)
    {
        bullet.state = 1;
        bullet.spawnTime = totalTime;
        
        
        // ===============================================
        // SPAWNER
        // ===============================================
        
        bullet.posX = originX;
        bullet.posY = originY;
        
        float u1 = max(0.00001f, hash(index + (uint) (totalTime * 1000.0f)));
        float u2 = hash(index + 1337U + (uint) (totalTime * 1000.0f));
        
        float z0 = sqrt(-2.0f * log(u1)) * cos(6.28318f * u2); // Box-Mueller
        float z1 = sqrt(-2.0f * log(u1)) * sin(6.28318f * u2);
        
        float sway = sin(totalTime * (1.5f + chaosFactor)) * 0.7f;
        
        float baseAngle = -1.5708f + sway; // Directly downwards
        //float angle = baseAngle + (z0 * spatialSpread);
       
        float streamCount = 5.0f;
        float streamID = float(index % int(streamCount));
        float baseStreamAngle = baseAngle - 0.6f + (streamID / (streamCount - 1.0f)) * 1.2f;
        
        float angle = baseStreamAngle + (z0 * spatialSpread * 0.1f);
        
        float speed = 0.5f + (z1 * 0.05f) + (band2 * 2.5f);
        bullet.baseRadius = 0.1f + (band1 * 2.0f) * chaosFactor * deltaTime;
        
        // ===============================================
        // SPAWNER END
        // ===============================================
        if (stateID == 0)
        {
            // STATE 0
            /*
            float streamCount = 15.0f;
            float streamID = float(index % int(streamCount));
            float angleOffset = ((streamID / streamCount) - 0.5f) * 1.6f;
            angle += angleOffset + sin(totalTime * 1.0f) * 0.3f;
            */
            
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
            // STATE 2: CYMATIC FLOWER
            
            // Golden Ratio
            angle = float(index) * 2.39996f + (totalTime * 2.0f);
            
            // Flower Petals
            float petalCount = 5.0f;
            
            float petalShape = abs(sin(angle * petalCount));
            
            speed = 0.03f + (petalShape * 0.8f);
            
            speed += (band1 * 0.5f);
            
            bullet.baseRadius = 0.1f + (petalShape * 0.05f);
        }

        bullet.velX = cos(angle) * speed;
        bullet.velY = sin(angle) * speed;
        
        //bullet.baseRadius = 0.05f;
        
        Bullets[index] = bullet;
        return;
    }
    // 2. Skip if the bullet is dead
    if (bullet.state == 0)
        return;
    
    float airSpeedMultiplier = 0.5f + (band2 * 50.0f);
    
    bullet.posX += bullet.velX * airSpeedMultiplier * deltaTime;
    bullet.posY += bullet.velY * airSpeedMultiplier * deltaTime;

   if (abs(bullet.posX) > 2.0f || abs(bullet.posY) > 2.0f)
   {
        bullet.state = 0;
   }
    
    // 5. Write the updated bullet back to the VRAM
    Bullets[index] = bullet;
}