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

// Inputs

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    
    nointerpolation float baseRadius : RADIUS;
    nointerpolation uint state : STATE;
};

float4 DrawBullet(float2 uv)
{
    // 1. Center the UV
    float2 center = uv * 2.0 - 1.0;
    
    // 2. Calculate the distance from the center (for SDF)
    float distance = length(center);
    
    // 3. Bullet Core, Anti-Aliased Edge
    float coreRadius = 0.05 + (band3 * 0.05);
    float core = smoothstep(0.1, coreRadius * 0.2, distance); // < 0.1 opaque, > 0.02 transparent
    
    // 4. Glow Effect (Outer Ring)
    float dynamicGlow = 1.0 + chaosFactor * 4.5;
    float glow = exp(-distance * 18.0) * dynamicGlow; // Exponential decay for glow
    
    float3 baseColor = float3(1.0, 0.0, 0.25); // Pink color for the bullet    
    
    float3 finalColor = (baseColor * glow) + (float3(1.0, 0.8, 0.8) * core); // Modulate color by core and glow
    
    float alpha = 0.5 - smoothstep(0.95, 1.0, distance); // Alpha based on distance for anti-aliasing
    
    finalColor *= alpha; // Modulate final color by alpha for smooth edges
    
    return float4(finalColor, alpha);    
}

float4 DrawPlayer(float2 uv, float exactHitRadius, float quadBaseRadius)
{
    // 1. Center the UV
    float2 center = uv * 2.0 - 1.0;
    
    // 2. DIAMOND SDF: MANHATTAN DISTANCE
    float diamondDistance = abs(center.x) + abs(center.y);
    
    // 3. Glow Effect (Inner Ring) & Color
    float glow = pow(diamondDistance, 1.5) * 1.5;
    float3 baseColor = float3(0.5, 0.0, 0.75);
    
    // 4. Core Effect
    float hitUVRadius = exactHitRadius / quadBaseRadius;
    float euclideanDistance = length(center);
    
    float hitCore = smoothstep(hitUVRadius, hitUVRadius * 0.2, euclideanDistance);
    float3 coreColor = float3(0.0, 1.0, 0.1);
    
    // 5. Combine Effects
    float3 finalColor = (baseColor * glow) + (coreColor * hitCore);
    
    // 6. Alpha for Anti-Aliasing based on Diamond Distance
    //float alpha = 1.0 - smoothstep(0.95, 1.0, diamondDistance);
    float mask = 1.0 - smoothstep(0.9, 1.0, diamondDistance);
    float alpha = mask * saturate(glow + hitCore);
    
    // 7. Modulate final color by alpha for smooth edges
    alpha = max(alpha, hitCore);
    
    finalColor *= alpha;
    
    return float4(finalColor, alpha);
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    if (input.state == 99)
    {
        return DrawPlayer(input.uv, hitRadius, input.baseRadius);
    }
    else
    {
        return DrawBullet(input.uv);
    }    
}