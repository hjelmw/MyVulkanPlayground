#version 450

layout(location = 0) in  float fragHeight;
layout(location = 1) in  vec3  fragColor;

layout(location = 0) out vec4  outColor;

void main() 
{
    float greyscaleColor = (fragHeight + 16) / 32.0f;

    outColor = vec4(greyscaleColor, greyscaleColor, greyscaleColor, 1.0f);
}