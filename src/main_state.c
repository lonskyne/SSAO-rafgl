#include <main_state.h>
#include <glad/glad.h>
#include <math.h>

#include <rafgl.h>

#include <game_constants.h>
#include <sys/types.h>

#define KERNEL_SAMPLES 64

static rafgl_meshPUN_t meshes[6];

static vec3_t object_colour = RAFGL_BLUE;
static vec3_t light_colour = RAFGL_WHITE;
static vec3_t light_direction = vec3m(-0.55f, -0.55f, -0.63f);
static vec3_t ambient = RAFGL_GRAYX(0.3f);

#define NUM_SHADERS 1
static GLuint object_shader[NUM_SHADERS], object_uni_M[NUM_SHADERS], object_uni_VP[NUM_SHADERS], object_uni_object_colour[NUM_SHADERS], object_uni_light_colour[NUM_SHADERS], object_uni_light_direction[NUM_SHADERS], object_uni_ambient[NUM_SHADERS], object_uni_camera_position[NUM_SHADERS];

static const char *mesh_names[6] = {"res/models/monkey-adv.obj", "res/models/monkey-subdiv.obj", "res/models/suzanne.obj", "res/models/bunny.obj", "res/models/armadillo.obj", "res/models/dragon.obj"};

static int current_shader = 0;

static int num_meshes;

static rafgl_texture_t skybox_tex;

static GLuint g_buffer_shader, skybox_shader, skybox_shader_cell, ssao_shader, ssao_blur_shader;
static GLuint g_buffer_uni_M, g_buffer_uni_VP, skybox_uni_P, skybox_uni_V, ssao_buffer_uni_P, ssao_buffer_uni_M , ssao_buffer_uni_V, ssao_blur_buffer_uni_M, ssao_blur_buffer_uni_VP;
static GLuint skybox_cell_uni_P, skybox_cell_uni_V;

static GLuint uni_visibility_factor;

static rafgl_meshPUN_t skybox_mesh;

static rafgl_framebuffer_simple_t fbo, ssao_buffer, ssao_blur_buffer;
static rafgl_framebuffer_multitarget_t g_buffer;

GLuint uni_pos_slot, uni_norm_slot, uni_ssao_slot;
GLuint uni_pos_slot_ssao, uni_norm_slot_ssao, uni_noise_slot_ssao, uni_tex_slot_blur;


unsigned int noise_texture, off_ssao = 0, off_ssao_loc;
unsigned int scw_obj0, sch_obj0, scw_blur, sch_blur, scw_ssao, sch_ssao;
unsigned int screenW, screenH;


void main_state_init(GLFWwindow *window, void *args, int width, int height)
{
    screenW = width;
    screenH = height;

    // G Buffer setup
    g_buffer = rafgl_framebuffer_multitarget_create(width, height, 2);
    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.fbo_id);

    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);

    g_buffer_shader = rafgl_program_create_from_name("g_buffer_shader");
    g_buffer_uni_M = glGetUniformLocation(g_buffer_shader, "uni_M");
    g_buffer_uni_VP = glGetUniformLocation(g_buffer_shader, "uni_VP");

    // SSAO buffer setup
    ssao_buffer = rafgl_framebuffer_simple_create(width, height, GL_RGB);
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_buffer.fbo_id);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // SSAO blur buffer setup
    ssao_blur_buffer = rafgl_framebuffer_simple_create(width, height, GL_RGB);
    ssao_blur_shader = rafgl_program_create_from_name("ssao_blur_shader");
    uni_tex_slot_blur = glGetUniformLocation(ssao_blur_shader, "tex");

    ssao_blur_buffer_uni_M = glGetUniformLocation(ssao_blur_shader, "uni_M");
    ssao_blur_buffer_uni_VP = glGetUniformLocation(ssao_blur_shader, "uni_VP");

    scw_blur = glGetUniformLocation(ssao_blur_shader, "sc_width");
    sch_blur = glGetUniformLocation(ssao_blur_shader, "sc_height");

    
    // Main buffer and skybox setup
    fbo = rafgl_framebuffer_simple_create(width, height, GL_RGB);

    rafgl_texture_load_cubemap_named(&skybox_tex, "above_the_sea", "jpg");
    skybox_shader = rafgl_program_create_from_name("skybox_shader");
    skybox_shader_cell = rafgl_program_create_from_name("skybox_shader_cell");

    skybox_uni_P = glGetUniformLocation(skybox_shader, "uni_P");
    skybox_uni_V = glGetUniformLocation(skybox_shader, "uni_V");

    skybox_cell_uni_P = glGetUniformLocation(skybox_shader_cell, "uni_P");
    skybox_cell_uni_V = glGetUniformLocation(skybox_shader_cell, "uni_V");

    // Set up ssao shader
    ssao_shader = rafgl_program_create_from_name("ssao_shader");

    ssao_buffer_uni_M = glGetUniformLocation(ssao_shader, "uni_M");
    ssao_buffer_uni_P = glGetUniformLocation(ssao_shader, "uni_P");
    ssao_buffer_uni_V = glGetUniformLocation(ssao_shader, "uni_V");

    uni_pos_slot_ssao = glGetUniformLocation(ssao_shader, "g_position");
    uni_norm_slot_ssao = glGetUniformLocation(ssao_shader, "g_normal");
    uni_noise_slot_ssao = glGetUniformLocation(ssao_shader, "noise_tex");

    scw_ssao = glGetUniformLocation(ssao_shader, "sc_width");
    sch_ssao = glGetUniformLocation(ssao_shader, "sc_height");

    // Generating sample kernels
    GLfloat sample[KERNEL_SAMPLES * 3];

    for(unsigned int i = 0; i < KERNEL_SAMPLES; i++) {
        sample[i * 3 + 0] = randf() * 2 - 1.0;
        sample[i * 3 + 1] = randf() * 2 - 1.0;
        sample[i * 3 + 2] = randf();

        // normalize
        float vec_len = sqrt(sample[i * 3 + 0] * sample[i * 3 + 0] +
                             sample[i * 3 + 1] * sample[i * 3 + 1] +
                             sample[i * 3 + 2] * sample[i * 3 + 2]);

        sample[i * 3 + 0] /= vec_len;
        sample[i * 3 + 1] /= vec_len;
        sample[i * 3 + 2] /= vec_len;

        float rand_float = randf();
        sample[i * 3 + 0] *= rand_float;
        sample[i * 3 + 1] *= rand_float;
        sample[i * 3 + 2] *= rand_float;

        float scale = (float)i / (float)KERNEL_SAMPLES;
        float lf = rafgl_lerpf(0.1, 1.0f, scale * scale);
        
        sample[i * 3 + 0] *= lf;
        sample[i * 3 + 1] *= lf;
        sample[i * 3 + 2] *= lf;
    }

    GLuint vec_loc = glGetUniformLocation(ssao_shader, "samples");
    glUseProgram(ssao_shader);
    glUniform3fv(vec_loc, KERNEL_SAMPLES, sample);

    // Generate random rotation texture
    GLfloat ssao_noise[4][4][3];

    for (unsigned int i = 0; i < 4; i++)
    {
        for (unsigned int j = 0; j < 4; j++)
        {
            ssao_noise[i][j][0] = randf() * 2 + 1;
            ssao_noise[i][j][1] = randf() * 2 + 1;
            ssao_noise[i][j][2] = 0.0;
        }
    }

    glGenTextures(1, &noise_texture);
    glBindTexture(GL_TEXTURE_2D, noise_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssao_noise);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    rafgl_meshPUN_init(&skybox_mesh);
    rafgl_meshPUN_load_cube(&skybox_mesh, 1.0f);

    num_meshes = sizeof(mesh_names) / sizeof(mesh_names[0]);
    object_colour = vec3(0.8f, 0.40f, 0.0f);

    rafgl_log_fps(RAFGL_TRUE);

    for(int i = 0; i < num_meshes; i++)
    {
        rafgl_log(RAFGL_INFO, "Loading mesh %d!\n", i + 1);
        rafgl_meshPUN_init(meshes + i);
        rafgl_meshPUN_load_from_OBJ(meshes + i, mesh_names[i]);
    }


    char shader_name[128];

    for(int i = 0; i < NUM_SHADERS; i++)
    {
        sprintf(shader_name, "object_shader%d", i);
        object_shader[i] = rafgl_program_create_from_name(shader_name);
        object_uni_M[i] = glGetUniformLocation(object_shader[i], "uni_M");
        object_uni_VP[i] = glGetUniformLocation(object_shader[i], "uni_VP");
        object_uni_object_colour[i] = glGetUniformLocation(object_shader[i], "uni_object_colour");
        object_uni_light_colour[i] = glGetUniformLocation(object_shader[i], "uni_light_colour");
        object_uni_light_direction[i] = glGetUniformLocation(object_shader[i], "uni_light_direction");
        object_uni_ambient[i] = glGetUniformLocation(object_shader[i], "uni_ambient");
        object_uni_camera_position[i] = glGetUniformLocation(object_shader[i], "uni_camera_position");
        off_ssao_loc = glGetUniformLocation(object_shader[i], "off_ssao");

        uni_pos_slot = glGetUniformLocation(object_shader[i], "g_position");
        uni_norm_slot = glGetUniformLocation(object_shader[i], "g_normal");
        uni_ssao_slot = glGetUniformLocation(object_shader[i], "ssao_tex");

        scw_obj0 = glGetUniformLocation(object_shader[i], "sc_width");
        sch_obj0 = glGetUniformLocation(object_shader[i], "sc_height");
    }


    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    light_direction = v3_norm(light_direction);

    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
}


mat4_t model, view, projection, view_projection;

/* field of view */
float fov = 75.0f;

vec3_t camera_position = vec3m(0.0f, 1.0f, 6.5f);
vec3_t camera_target = vec3m(0.0f, 0.0f, 0.0f);
vec3_t camera_up = vec3m(0.0f, 1.0f, 0.0f);
vec3_t aim_dir = vec3m(0.0f, 0.0f, -1.0f);

float camera_angle = -M_PIf * 0.5f;
float angle_speed = 0.2f * M_PIf;
float move_speed = 2.4f;

float hoffset = 0;

int rotate = 0;
float model_angle = 0.0f;

void v3show(vec3_t v)
{
    printf("(%.2f %.2f %.2f)\n", v.x, v.y, v.z);
}

float time = 0.0f;
int reshow_cursor_flag = 0;
int last_lmb = 0;


int selected_mesh = 3;
float sensitivity = 1.0f;

int selected_shader = 0;

float visibility_factor = -2.0f;
float turn = 1.0f;

int num_key_down = 0;

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args)
{
    visibility_factor += delta_time * turn;
    if(visibility_factor > 12.0f) turn = -1;
    if(visibility_factor < -5.0f) turn = 1;

    time += delta_time;
    model_angle += delta_time * rotate;

    if(game_data->keys_down[RAFGL_KEY_0])
        num_key_down = 0;
    else if(game_data->keys_down[RAFGL_KEY_1])
        num_key_down = 1;
    else if(game_data->keys_down[RAFGL_KEY_2])
        num_key_down = 2;
    else if(game_data->keys_down[RAFGL_KEY_3])
        num_key_down = 3;
    else if(game_data->keys_down[RAFGL_KEY_4])
        num_key_down = 4;

    if(game_data->keys_down[RAFGL_KEY_P])
        off_ssao = 1;
    else
        off_ssao = 0;



    if(!game_data->keys_down[RAFGL_KEY_LEFT_SHIFT])
    {
        angle_speed = 0.2f * M_PIf;
        move_speed = 2.4f;
    }
    else
    {
        angle_speed = 3 * 0.2f * M_PIf;
        move_speed = 3 * 2.4f;
    }


    if(game_data->is_lmb_down)
    {

        if(reshow_cursor_flag == 0)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }

        float ydelta = game_data->mouse_pos_y - game_data->raster_height / 2;
        float xdelta = game_data->mouse_pos_x - game_data->raster_width / 2;

        if(!last_lmb)
        {
            ydelta = 0;
            xdelta = 0;
        }

        hoffset -= sensitivity * ydelta / game_data->raster_height;
        camera_angle += sensitivity * xdelta / game_data->raster_width;

        glfwSetCursorPos(window, game_data->raster_width / 2, game_data->raster_height / 2);
        reshow_cursor_flag = 1;
    }
    else if(reshow_cursor_flag)
    {
        reshow_cursor_flag = 0;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    last_lmb = game_data->is_lmb_down;

    aim_dir = v3_norm(vec3(cosf(camera_angle), hoffset, sinf(camera_angle)));

    if(game_data->keys_down['W']) camera_position = v3_add(camera_position, v3_muls(aim_dir, move_speed * delta_time));
    if(game_data->keys_down['S']) camera_position = v3_add(camera_position, v3_muls(aim_dir, -move_speed * delta_time));

    vec3_t right = v3_cross(aim_dir, vec3(0.0f, 1.0f, 0.0f));
    if(game_data->keys_down['D']) camera_position = v3_add(camera_position, v3_muls(right, move_speed * delta_time));
    if(game_data->keys_down['A']) camera_position = v3_add(camera_position, v3_muls(right, -move_speed * delta_time));

    if(game_data->keys_pressed['R']) rotate = !rotate;

    if(game_data->keys_pressed[RAFGL_KEY_KP_ADD]) selected_mesh = (selected_mesh + 1) % num_meshes;
    if(game_data->keys_pressed[RAFGL_KEY_KP_SUBTRACT]) selected_mesh = (selected_mesh + num_meshes - 1) % num_meshes;

    if(game_data->keys_down[RAFGL_KEY_ESCAPE]) glfwSetWindowShouldClose(window, GLFW_TRUE);

    if(game_data->keys_down[RAFGL_KEY_SPACE]) camera_position.y += 1.0f * delta_time * move_speed;
    if(game_data->keys_down[RAFGL_KEY_LEFT_CONTROL]) camera_position.y -= 1.0f * delta_time * move_speed;


    for(int i = 0; i < NUM_SHADERS; i++)
    {
        if(selected_shader != i && game_data->keys_pressed[RAFGL_KEY_0 + i])
        {
            selected_shader = i;
        }
    }

    float aspect = ((float)(game_data->raster_width)) / game_data->raster_height;
    projection = m4_perspective(fov, aspect, 0.1f, 100.0f);

    view = m4_look_at(camera_position, v3_add(camera_position, aim_dir), camera_up);

    model = m4_identity();
    model = m4_rotation_y(model_angle);
    model = m4_mul(model, m4_translation(vec3(0.0f, sinf(model_angle) * 0.45, 0.0f)));

    view_projection = m4_mul(projection, view);
}


void main_state_render(GLFWwindow *window, void *args)
{
    // Geometry pass
    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.fbo_id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(g_buffer_shader);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindVertexArray(meshes[selected_mesh].vao_id);

    glUniformMatrix4fv(g_buffer_uni_M, 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(g_buffer_uni_VP, 1, GL_FALSE, (void*) view_projection.m);

    glDrawArrays(GL_TRIANGLES, 0, meshes[selected_mesh].vertex_count);

    glBindVertexArray(0);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Calculate SSAO texture
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_buffer.fbo_id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ssao_shader);

    glUniform1i(uni_pos_slot_ssao, 0);
    glUniform1i(uni_norm_slot_ssao, 1);
    glUniform1i(uni_noise_slot_ssao, 2);

    glUniform1i(scw_ssao, screenW);
    glUniform1i(sch_ssao, screenH);

    // Position texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_buffer.tex_ids[0]);
    glGenerateMipmap(GL_TEXTURE_2D);
    // Normal texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_buffer.tex_ids[1]);
    glGenerateMipmap(GL_TEXTURE_2D);
    // Noise texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noise_texture);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindVertexArray(meshes[selected_mesh].vao_id);

    glUniformMatrix4fv(ssao_buffer_uni_M, 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(ssao_buffer_uni_P, 1, GL_FALSE, (void*) projection.m);
    glUniformMatrix4fv(ssao_buffer_uni_V, 1, GL_FALSE, (void*) view.m);


    glDrawArrays(GL_TRIANGLES, 0, meshes[selected_mesh].vertex_count);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Blur SSAO texture
    glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_buffer.fbo_id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(ssao_blur_shader);

    glUniform1i(uni_tex_slot_blur, 0);

    glUniform1i(scw_blur, screenW);
    glUniform1i(sch_blur, screenH);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssao_buffer.tex_id);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindVertexArray(meshes[selected_mesh].vao_id);

    glUniformMatrix4fv(ssao_blur_buffer_uni_M, 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(ssao_blur_buffer_uni_VP, 1, GL_FALSE, (void*) view_projection.m);

    glDrawArrays(GL_TRIANGLES, 0, meshes[selected_mesh].vertex_count);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // Skybox
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo_id);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthMask(GL_FALSE);

    glUseProgram(skybox_shader);
    glUniformMatrix4fv(skybox_uni_V, 1, GL_FALSE, (void*) view.m);
    glUniformMatrix4fv(skybox_uni_P, 1, GL_FALSE, (void*) projection.m);

    glBindVertexArray(skybox_mesh.vao_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex.tex_id);

    glDrawArrays(GL_TRIANGLES, 0, skybox_mesh.vertex_count);
    glDepthMask(GL_TRUE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);


    // Lightning pass
    glUseProgram(object_shader[selected_shader]);

    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_tex.tex_id);

    glUniform1i(uni_pos_slot, 0);
    glUniform1i(uni_norm_slot, 1);
    glUniform1i(uni_ssao_slot, 2);

    glUniform1i(scw_obj0, screenW);
    glUniform1i(sch_obj0, screenH);

    // Position texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_buffer.tex_ids[0]);
    glGenerateMipmap(GL_TEXTURE_2D);
    // Normal texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_buffer.tex_ids[1]);
    glGenerateMipmap(GL_TEXTURE_2D);
    // SSAO texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssao_blur_buffer.tex_id);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindVertexArray(meshes[selected_mesh].vao_id);

    glUniformMatrix4fv(object_uni_M[selected_shader], 1, GL_FALSE, (void*) model.m);
    glUniformMatrix4fv(object_uni_VP[selected_shader], 1, GL_FALSE, (void*) view_projection.m);

    glUniform3f(object_uni_object_colour[selected_shader], object_colour.x, object_colour.y, object_colour.z);
    glUniform3f(object_uni_light_colour[selected_shader], light_colour.x, light_colour.y, light_colour.z);
    glUniform3f(object_uni_light_direction[selected_shader], light_direction.x, light_direction.y, light_direction.z);
    glUniform3f(object_uni_ambient[selected_shader], ambient.x, ambient.y, ambient.z);
    glUniform3f(object_uni_camera_position[selected_shader], camera_position.x, camera_position.y, camera_position.z);
    glUniform1i(off_ssao_loc, off_ssao);

    glDrawArrays(GL_TRIANGLES, 0, meshes[selected_mesh].vertex_count);

    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);

    rafgl_texture_t tmptex;

    if(num_key_down == 1)
        tmptex.tex_id = g_buffer.tex_ids[0];
    else if(num_key_down == 2)
        tmptex.tex_id = g_buffer.tex_ids[1];
    else if(num_key_down == 3)
        tmptex.tex_id = ssao_buffer.tex_id;
    else if(num_key_down == 4)
        tmptex.tex_id = ssao_blur_buffer.tex_id;
    else
        tmptex.tex_id = fbo.tex_id;

    rafgl_texture_show(&tmptex, 1);
    
    glEnable(GL_DEPTH_TEST);

}


void main_state_cleanup(GLFWwindow *window, void *args)
{
    glDeleteShader(ssao_shader);
    glDeleteShader(ssao_blur_shader);
    glDeleteShader(g_buffer_shader);
    glDeleteShader(skybox_shader);
    glDeleteShader(skybox_shader_cell);
    glDeleteShader(object_shader[0]);
}