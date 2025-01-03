#version 330

out vec4 final_colour;

in vec3 tex_coords;

uniform samplerCube skybox;

void main()
{   
	vec3 col = texture(skybox, tex_coords).rgb;
	
	float bright = (col.r + col.g + col.b) / 3.0;
	
	float cell_steps = 10.0;
	bright = floor(bright * cell_steps) / cell_steps;
	
	col = vec3(bright);
    final_colour = vec4(col, 1.0);
}