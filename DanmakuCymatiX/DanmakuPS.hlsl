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

// Inputs

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    // 1. Center the UV
    float2 center = input.uv * 2.0 - 1.0;
    
    // 2. Calculate the distance from the center (for SDF)
    float distance = length(center);
    
    // 3. Bullet Core, Anti-Aliased Edge
    float core = smoothstep(0.05, 0.0075, distance); // < 0.1 opaque, > 0.02 transparent
    
    // 4. Glow Effect (Outer Ring)
    float glow = exp(-distance * 10.0) * 0.75; // Exponential decay for glow
    
    // MVP: COLOR IS FIXED FOR NOW!!
    //float4 color = float4(core + glow, core + glow, core + glow, 1.0);
    
    float3 baseColor = float3(1.0, 0.8, 0.8); // Pink color for the bullet
    
    float3 finalColor = (baseColor * glow) + (float3(0.0, 1.0, 0.2) * core); // Modulate color by core and glow
    
    float alpha = 1.0 - smoothstep(0.95, 1.0, distance); // Alpha based on distance for anti-aliasing
    
    finalColor *= alpha; // Modulate final color by alpha for smooth edges
    
    return float4(finalColor, alpha);
}