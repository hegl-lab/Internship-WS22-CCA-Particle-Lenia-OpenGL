// FILE: shaders/particle-lenia/particle_2d.vert
// this file get's prefixed with fields_functions_2d.glsl

// value for gradient calculations
uniform float h;
uniform float h2;
// time step size
uniform float dt;

uniform float translate_x;
uniform float translate_y;

layout (std430) restrict buffer ParticlesBufferUpdated {
    vec2 particles_updated[];
};

// calculates the gradient of the energy field
// at the given postion
vec2 gradient(vec2 position) {
    float e1 = fields(position + vec2(h, 0)).a;
    float e2 = fields(position - vec2(h, 0)).a;
    float e3 = fields(position + vec2(0, h)).a;
    float e4 = fields(position - vec2(0, h)).a;

    return vec2(
        (e1 - e2) / h2,
        (e3 - e4) / h2
    );
}

void main()
{
    // get particle based on the id of the vertex
    vec2 position = particles[gl_VertexID];
    position -= dt * gradient(position);
    particles_updated[gl_VertexID] = position;

    gl_Position.xyz = vec3((position.x - translate_x) / internal_width, (position.y - translate_y) / internal_height, 0.0);
}