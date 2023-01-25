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
// FILE: shaders/particle-lenia/3d/fields_3d.frag
// this file get's prefixed with fields_functions_3d.glsl

out vec4 FragColor;
in vec2 TexCoord;

vec4 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    vec4 final = (c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0))).rgbr;
    final.a = 1.0;
    return final;
}

//uniform vec4 color1 = vec4(1.0, 0.78, 0.349, 1.0);
//uniform vec4 color2 = vec4(0.702, 0.949, 0.867, 1.0);
//uniform vec4 color1 = vec4(37. / 255., 77. / 255., 25. / 255., 1.0);
//uniform vec4 color2 = vec4(1.00, 0.11, 0.11, 1.0);
vec4 convert_rgb(vec3 a) {
    return vec4(a.r / 255., a.g / 255., a.b / 255., 1.0);
}

uniform vec4 background_color = vec4(1 / 255., 23 / 255., 47 / 255., 1.0);
uniform vec4 color1 = vec4(46 / 255., 134 / 255., 171 / 255., 1.0);
uniform vec4 color2 = vec4(241.0 / 255., 143.0 / 255., 1.0 / 255., 1.0);

// select fields to display
// 0: none
// 1: U, 2: R, 3: G, 4: E
uniform int render_1 = -1;
uniform int render_2 = -1;

// translate information
uniform float translate_x;
uniform float translate_y;
uniform float depth;

vec4 blend(vec4 color1, vec4 color2, float amount) {
    return (1.0 - amount) * color1 + amount * color2;
}

void main()
{
    if (render_1 == 0 && render_2 == 0) {
        FragColor = background_color;
    } else {
        vec3 position = (vec3(TexCoord.x * 2. - 1., TexCoord.y * 2. - 1., 0.) / scale) * rotation - translate;
        vec3 direction = rotation * vec3(0, 0, 1);

        FragColor = background_color;

        vec4 fields = fields(position + depth * direction);

        float value_1 = 0.0;
        switch (render_1) {
            case 1:
                value_1 = min(fields.x, 1.0);
                break;
            case 2:
                value_1 = min(fields.y, 1.0);
                break;
            case 3:
                value_1 = min(fields.z, 1.0);
                break;
            case 4:
                value_1 = min(abs(fields.a), 1.0);
                break;
        }

        float value_2 = 0.0;
        switch (render_2) {
            case 1:
                value_2 = min(fields.x, 1.0);
                break;
            case 2:
                value_2 = min(fields.y, 1.0);
                break;
            case 3:
                value_2 = min(fields.z, 1.0);
                break;
            case 4:
                value_2 = min(abs(fields.a), 1.0);
                break;
        }

        FragColor =
        blend(
            blend(
                FragColor, color1, value_1
            ),
            color2, value_2
        );
    }
}