#version 330

out vec4 final_colour;

uniform sampler2D tex;

void main()
{
	vec2 tex_coords = vec2(gl_FragCoord.x / 1280, gl_FragCoord.y / 720);

    float result = 0.0;

    for (int x = -2; x < 2; ++x) 
        for (int y = -2; y < 2; ++y) 
            result += texture(tex, vec2(tex_coords.x + float(x) / 1280, tex_coords.y + float(y) / 720)).r;

    final_colour = vec4(vec3(result / (4.0 * 4.0)), 1.0);
}