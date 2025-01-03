#version 330

in vec3 pass_normal;
in vec3 pass_world_position;

out vec4 final_colour;

uniform vec3 uni_object_colour;
uniform vec3 uni_light_colour;
uniform vec3 uni_light_direction;
uniform vec3 uni_ambient;
uniform vec3 uni_camera_position;


void main()
{
	vec3 view_vector = normalize(pass_world_position - uni_camera_position);
	vec3 neg_view_vector = view_vector * (-1);
	vec3 normalized_normal = normalize(pass_normal);
	vec3 reflected_light = reflect(uni_light_direction, normalized_normal);
	
	float specular_factor = clamp(dot(reflected_light, neg_view_vector), 0, 1);
	specular_factor = pow(specular_factor, 5);
	vec3 specular_colour = uni_light_colour * specular_factor;
	
	float light_factor = clamp(dot(normalize(pass_normal), normalize(uni_light_direction * (-1))), 0, 1);
    vec3 diffuse_colour = (uni_ambient * uni_object_colour) + (uni_object_colour * (uni_light_colour * light_factor));
	
	final_colour = vec4(diffuse_colour + specular_colour, 1.0);
}