// Inputs

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float uv : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    // 1. Center the UV
    float2 center = input.uv * 2.0 - 1.0;
    
    // 2. Calculate the distance from the center (for SDF)
    float distance = length(center);
    
    // 3. Bullet Core, Anti-Aliased Edge
    float core = smoothstep(0.9, 0.7, distance); // < 0.7 opaque, > 0.9 transparent
    
    // 4. Glow Effect (Outer Ring)
    float glow = exp(-distance * 3.0); // Exponential decay for glow
    
    // MVP: COLOR IS FIXED FOR NOW!!
    // float4 color = float4(core + glow, core + glow, core + glow, 1.0);
    
    float3 baseColor = float3(1.0, 0.5, 0.0); // Orange color for the bullet
    
    float3 finalColor = (baseColor * glow) + (float3(1.0, 1.0, 1.0) * core); // Modulate color by core and glow
    
    float alpha = 1.0 - smoothstep(0.95, 1.0, distance); // Alpha based on distance for anti-aliasing
    
    return float4(finalColor, alpha);
}