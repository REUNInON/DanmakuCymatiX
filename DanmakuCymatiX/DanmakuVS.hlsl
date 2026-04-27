// Inputs
struct VS_INPUT
{
    uint instanceID : SV_InstanceID;
    uint vertexID : SV_VertexID;
};

// Outputs
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    
    nointerpolation float baseRadius : RADIUS;
    nointerpolation uint state : STATE;
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

cbuffer GlobalConstants : register(b0)
{
    float screenWidth;
    float screenHeight;
    uint packedStateAndSpawn;
    uint spawnStartIndex;
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

StructuredBuffer<BulletGPU> instanceBuffer : register(t0);

static const float4 quadVertices[6] =
{
    float4(-1.0, -1.0, 0.0, 1.0),
    float4(1.0, -1.0, 1.0, 1.0),
    float4(-1.0, 1.0, 0.0, 0.0),
    float4(-1.0, 1.0, 0.0, 0.0),
    float4(1.0, -1.0, 1.0, 1.0),
    float4(1.0, 1.0, 1.0, 0.0)
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    
    BulletGPU bullet = instanceBuffer[input.instanceID];
    
    // Do not render if the bullet is dead
    if (bullet.state == 0)
    {
        output.position = float4(0.0, 0.0, 0.0, 0.0);
        output.uv = float2(0.0, 0.0);
        return output;
    }
    
    // Find the current position of the bullet
    float4 quadData = quadVertices[input.vertexID];
    
    output.uv = quadData.zw; // UV coordinates from the vertex data
    
    // Fix the aspect ratio
    float aspectRatio = screenWidth / screenHeight;
    
    float2 localPos;
    localPos.x = (quadData.x * bullet.baseRadius) / aspectRatio;
    localPos.y = (quadData.y * bullet.baseRadius);
    
    // WORLD COORDINATES OF THE BULLET
    output.position = float4(bullet.posX + localPos.x, bullet.posY + localPos.y, 0.0, 1.0);
    
    output.state = bullet.state;
    output.baseRadius = bullet.baseRadius;
    
    return output;
}