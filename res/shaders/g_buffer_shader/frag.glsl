#version 330

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;

in vec3 pass_world_position;
in vec3 pass_normal;

void main()
{
	g_position = pass_world_position;
	g_normal = normalize(pass_normal);
}