#version 330

out vec4 final_colour;

in vec3 tex_coords;

uniform samplerCube skybox;

void main()
{    
    final_colour = texture(skybox, tex_coords);
}