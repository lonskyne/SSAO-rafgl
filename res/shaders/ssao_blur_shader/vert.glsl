#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;

uniform mat4 uni_M;
uniform mat4 uni_VP;

void main()
{
	vec4 world_position = uni_M * vec4(position, 1.0);	
	gl_Position = uni_VP * world_position;
}