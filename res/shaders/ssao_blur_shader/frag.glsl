#version 330

out vec4 final_colour;

uniform sampler2D tex;

uniform int sc_width;
uniform int sc_height;

void main()
{
	vec2 tex_coords = vec2(gl_FragCoord.x / sc_width, gl_FragCoord.y / sc_height);

    float result = 0.0;

    for (int x = -2; x < 2; ++x) 
        for (int y = -2; y < 2; ++y) 
            result += texture(tex, vec2(tex_coords.x + float(x) / sc_width, tex_coords.y + float(y) / sc_height)).r;

    final_colour = vec4(vec3(result / (4.0 * 4.0)), 1.0);
}