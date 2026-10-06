#version 450

layout(location = 0) in  vec3  inWorldPosition;
layout(location = 1) in  vec3  inNormal;

// Same G-Buffer layout as geometry.frag
layout(location = 0) out vec4  outPosition;
layout(location = 1) out vec4  outNormal;
layout(location = 2) out vec4  outAlbedo;

void main()
{
    vec3 normal = normalize(inNormal);

    // Heightmap texel / 256 (TerrainNode.cpp uses height scale 64/256 and shift 16), 0 is sea level
    float height = clamp((inWorldPosition.y + 16.0f) / 64.0f, 0.0f, 1.0f);

    vec3 albedo = mix(vec3(0.22f, 0.35f, 0.15f), vec3(0.40f, 0.36f, 0.30f), smoothstep(0.02f, 0.30f, height)); // grass -> rock
    albedo      = mix(albedo, vec3(0.30f, 0.29f, 0.27f), 1.0f - smoothstep(0.55f, 0.85f, normal.y));            // steep slopes -> rock
    albedo      = mix(albedo, vec3(0.95f, 0.95f, 0.97f), smoothstep(0.45f, 0.65f, height));                      // snow caps
    if (height <= 0.0f)
    {
        albedo = vec3(0.05f, 0.15f, 0.35f); // sea
    }

    outPosition = vec4(inWorldPosition, 1.0f);
    outNormal   = vec4(normal, 1.0f);
    outAlbedo   = vec4(albedo, 1.0f);
}