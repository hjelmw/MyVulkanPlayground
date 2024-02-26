#version 450

layout(location = 0) out vec4 color;

void main()
{
    gl_FragDepth = gl_FragCoord.z;
}