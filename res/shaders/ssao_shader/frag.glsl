#version 330

in mat4 projection;

out vec4 final_colour;

uniform vec3 samples[64];

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D noise_tex;

const vec2 noise_scale = vec2(1280.0/4.0, 720.0/4.0);
const float radius = 0.5;
const float bias = 0.025;

void main()
{
	// vec2 tex_coords = vec2(gl_FragCoord.x / 1280, gl_FragCoord.y / 720);

	// vec3 world_position = texture(g_position, tex_coords).xyz;
	// vec3 normal = normalize(texture(g_normal, tex_coords).rgb);
	// vec3 random_vec = normalize(texture(noise_tex, tex_coords * noise_scale).xyz);
	
	// vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
	// vec3 bitangent = cross(normal, tangent);
	// mat3 TBN = mat3(tangent, bitangent, normal);

	// float occlusion = 0.0;
	// for(int i = 0; i < 64; ++i)
	// {
	// 	vec3 sample_pos = TBN * samples[i]; // from tangent space to view space
	// 	sample_pos = world_position + sample_pos * radius; 
		
	// 	vec4 offset = vec4(sample_pos, 1.0);
	// 	offset = projection * offset;    // from view to clip-space
	// 	offset.xyz /= offset.w;               // perspective divide
	// 	offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0 

	// 	float sample_depth = texture(g_position, offset.xy).z; 

	// 	float range_check = smoothstep(0.0, 1.0, radius / abs(world_position.z - sample_depth));
	// 	occlusion += (sample_depth >= sample_pos.z + bias ? 1.0 : 0.0) * range_check;
	// }

	// occlusion = 1.0 - (occlusion / 64);
	final_colour = vec4(1.0); 
}