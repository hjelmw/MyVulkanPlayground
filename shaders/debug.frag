#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout (location = 1) in vec4 inFragColor;
layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = inFragColor;
}