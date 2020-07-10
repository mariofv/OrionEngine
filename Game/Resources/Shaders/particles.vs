layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec2 vertex_uv0;

layout (std140) uniform Matrices
{
  mat4 model;
  mat4 proj;
	mat4 view;
} matrices;


struct Billboard
{
  sampler2D texture;

  int num_columns;
  int num_rows;
};
uniform Billboard billboard;

out vec2 texCoord;
out vec4 color;

struct Particle
{
    vec4 position_initial;
    vec4 position;
    vec4 velocity_initial;
    vec4 velocity;
    vec4 color;

    float particle_scale;
    float time_passed;
    float life;
    float time_counter;

    float X;
    float Y;
    float width;
    float height;

    mat4 model;
    mat4 geometric_space;
};


layout(std430, binding = 2) buffer particles_data
{
    Particle particles[];
};

mat4x4 GetModelViewMatrix();

void main()
{
    mat4x4 model_view_matrix = GetModelViewMatrix();
    gl_Position = matrices.proj *  model_view_matrix * vec4(vertex_position, 1.0);

    vec2 vertex_uvs = vertex_uv0;

    #if ENABLE_SPRITESHEET
      float u = mix(particles[gl_InstanceID].X, particles[gl_InstanceID].X + 1, vertex_uv0.x) / billboard.num_columns;
      float v = mix(particles[gl_InstanceID].Y, particles[gl_InstanceID].Y + 1, vertex_uv0.y) / billboard.num_rows;
      vertex_uvs = vec2(u, v);
    #endif

    texCoord = vertex_uvs;
    color = particles[gl_InstanceID].color;
}

mat4x4 GetModelViewMatrix()
{
  mat4x4 model_view = matrices.view * particles[gl_InstanceID].model;
  #if ENABLE_BILLBOARD_VIEWPOINT_ALIGNMENT
      model_view[0][0] = particles[gl_InstanceID].model[0][0];
      model_view[0][1] = 0.0;
      model_view[0][2] = 0.0;

      model_view[1][0] = 0.0;
      model_view[1][1] = particles[gl_InstanceID].model[1][1];
      model_view[1][2] = 0.0;

      model_view[2][0] = 0.0;
      model_view[2][1] = 0.0;
      model_view[2][2] = particles[gl_InstanceID].model[2][2];

  #elif ENABLE_BILLBOARD_AXIAL_ALIGNMENT
      model_view[0][0] = particles[gl_InstanceID].model[0][0];
      model_view[0][1] = 0.0;
      model_view[0][2] = 0.0;

      model_view[2][0] = 0.0;
      model_view[2][1] = 0.0;
      model_view[2][2] = particles[gl_InstanceID].model[2][2];

  #endif
  return model_view;
}
