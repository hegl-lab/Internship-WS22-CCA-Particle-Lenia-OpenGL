// FILE: shaders/particle-lenia/3d/fields_functions_3d.glsl
#version 430 core

uniform vec3 scale;
uniform vec3 translate;
uniform mat3 rotation;

// parameters for the kernel
uniform float w_k;
uniform float mu_k;
// sigma k squared
uniform float sigma_k2;

// parameters for the growth field
uniform float mu_g;
// sigma g squared
uniform float sigma_g2;

// factor used to scale repulsion
uniform float c_rep;
// minimum distance to particle for repulsion
uniform float r_distance;

// number of particles
uniform int num_particles;

layout (std430) restrict buffer ParticlesBuffer {
    vec3 particles[];
};

// calculates the euclidean norm of a vector
float euclid_norm(vec3 vec) {
    return sqrt(
        vec.x * vec.x + vec.y * vec.y + vec.z * vec.z
    );
}

// applies the kernel function to a value r
float K(float r) {
    return w_k *
    exp(
        -pow(r - mu_k, 2.0) / sigma_k2
    );
}

// calculates the value of the field U and the repulsion field R at the given position using the following formula
// U(position) = sum_(i=0)^(num_particles-1) K(||position - particle_i||
// R(position) = (c_rep / 2) * sum_(i in particles that aren't directly at position) max(1 - ||x - particle||, 0)^2
vec2 U_and_R(vec3 position) {
    float u = 0.0;
    float r = 0.0;
    for (int i = 0; i < num_particles; ++i) {
        float norm = euclid_norm(particles[i] - position);
        u += K(norm);
        if (norm >= r_distance) {
            r += pow(max(1.0 - norm, 0.0), 2.0);
        }
    }
    return vec2(u, r);
}

// calculates the value of the growth field G based on the field U using the following formula
// G(u) = exp(-(u - mu_G)^2 / sigma_G^2)
float G(float u) {
    return exp(
        -pow(u - mu_g, 2.0) / sigma_g2
    );
}

// calculates the value of the energy field based on the value of the repulsion and the growth field
float E(float r, float g) {
    return r - g;
}

// calculates the values of all fields and returns a vec4
// with the values in the following ortder: u, r, g, e
vec4 fields(vec3 position) {
    vec2 ur = U_and_R(position);
    float g = G(ur.x);
    float e = E(ur.y, g);
    return vec4(ur.x, ur.y, g, e);
}
// FILE: shaders/particle-lenia/3d/particle_3d.vert
// this file get's prefixed with fields_functions_3d.glsl

out vec4 generated_color;

// value for gradient calculations
uniform float h;
uniform float h2;
// time step size
uniform float dt;

layout (std430) restrict buffer ParticlesBufferUpdated {
    vec3 particles_updated[];
};

// calculates the gradient of the energy field
// at the given postion
vec3 gradient(vec3 position) {
    float e1 = fields(position + vec3(h, 0, 0)).a;
    float e2 = fields(position - vec3(h, 0, 0)).a;
    float e3 = fields(position + vec3(0, h, 0)).a;
    float e4 = fields(position - vec3(0, h, 0)).a;
    float e5 = fields(position + vec3(0, 0, h)).a;
    float e6 = fields(position - vec3(0, 0, h)).a;

    return vec3(
        (e1 - e2) / h2,
        (e3 - e4) / h2,
        (e5 - e6) / h2
    );
}

void main()
{
    // get particle based on the id of the vertex
    vec3 position = particles[gl_VertexID];
    position -= dt * gradient(position);
    particles_updated[gl_VertexID] = position;

    gl_Position.xyz = (rotation * (position - translate)) * scale;
    gl_PointSize = 5.0 * exp(-gl_Position.z);
}