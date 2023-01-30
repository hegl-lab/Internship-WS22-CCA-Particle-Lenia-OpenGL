#include <cmath>
#include <GLFWAbstraction.h>
#include <random>
#include <chrono>

class ParticleLenia2D {
public:
    float internal_width = 30;
    float internal_height = 30;

    int view_width = 900;
    int view_height = 900;

    // parameters for the kernel
    float w_k = 0.022;
    float mu_k = 4.0;
    // sigma k squared
    float sigma_k2 = 1.0;

    // parameters for the growth field
    float mu_g = 0.6;
    // sigma g squared
    float sigma_g2 = std::pow(0.15f, 2.0f);

    // factor used to scale repulsion
    float c_rep = 1.0;
    // minimum distance to particle for repulsion
    float r_distance = 1e-10;

    // number of particles
    int num_particles = 300;

    // value for gradient calculations
    float h = 0.01;
    float h2 = 2 * h;
    // time step size
    float dt = 0.1;

    // colors
    ImVec4 background_color = ImVec4(1 / 255., 23 / 255., 47 / 255., 1.0);
    ImVec4 color_1 = ImVec4(46 / 255., 134 / 255., 171 / 255., 1.0);
    ImVec4 color_2 = ImVec4(241.0 / 255., 143.0 / 255., 1.0 / 255., 1.0);
    int render_1 = 1;
    int render_2 = 3;

    float translate_x = 0;
    float translate_y = 0;

    bool is_particles_a = true;

    Buffer particles_a = Buffer(num_particles * 2, GL_SHADER_STORAGE_BUFFER);
    Buffer particles_b = Buffer(num_particles * 2, GL_SHADER_STORAGE_BUFFER);

    FragmentOnlyShader info_shader = FragmentOnlyShader("shaders/particle-lenia/2d/fields_2d.generated.frag");
    SimpleComputeShader particle_step = SimpleComputeShader("shaders/particle-lenia/2d/particle_2d.generated.comp");

    void init() {
        std::cout << "HI\n";
        particles_a.init();
        particles_b.init();

        // generate random particles
        reset_particles();

        info_shader.init_without_arguments();
        particle_step.init_without_arguments();
    }


    void reset_particles() {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_real_distribution<> distribution(-((double) internal_width) * 0.3,
                                                      ((double) internal_width) * 0.3);
        std::vector<float> particles;
        for (int i = 0; i < 2 * num_particles; ++i) {
            particles.emplace_back(distribution(rng));
        }
        particles_a.set_data(particles);
        particles_b.set_data(particles);
    }

    void resize_buffer(bool reset, bool append_random = true, ImVec2 append = {0, 0}) {
        std::vector<float> data;
        if (reset) {
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_real_distribution<> distribution(-((double) internal_width) * 0.3,
                                                          ((double) internal_width) * 0.3);
            for (int i = 0; i < 2 * num_particles; ++i) {
                data.emplace_back(distribution(rng));
            }
        } else {
            data = particles_a.get_data();

            // create more data if necessary
            if (data.size() < num_particles * 2) {
                if (append_random) {
                    std::random_device dev;
                    std::mt19937 rng(dev());
                    std::uniform_real_distribution<> distribution(-((double) internal_width) * 0.3,
                                                                  ((double) internal_width) * 0.3);
                    while (data.size() < num_particles * 2) data.push_back(distribution(rng));
                } else {
                    while (data.size() < num_particles * 2) {
                        data.push_back(append.x);
                        data.push_back(append.y);
                    }
                }
            } else {
                data.resize(2 * num_particles);
            }
        }

        particles_a.delete_buffer();
        particles_b.delete_buffer();

        particles_a = Buffer(2 * num_particles, GL_SHADER_STORAGE_BUFFER);
        particles_b = Buffer(2 * num_particles, GL_SHADER_STORAGE_BUFFER);

        particles_a.init();
        particles_b.init();

        particles_a.set_data(data);
        particles_b.set_data(data);
    }


    ImVec2 translate_mouse_position(bool include_translate = false) {
        ImVec2 position = ImGui::GetMousePos();

        position.x /= CURRENT_WIDTH;
        position.x *= 2 * internal_width;
        position.x -= internal_width;

        position.y /= CURRENT_HEIGHT;
        position.y = 1 - position.y;
        position.y *= 2 * internal_height;
        position.y -= internal_height;

        if (include_translate) {
            position.x += translate_x;
            position.y += translate_y;
        }

        return position;
    }

    void step(int steps_per_frame) {
        for (int i = 0; i < steps_per_frame; ++i) {
            if (is_particles_a) {
                particle_step.bind_buffer("ParticlesBuffer", particles_a, 0);
                particle_step.bind_buffer("ParticlesBufferUpdated", particles_b, 1);
            } else {
                particle_step.bind_buffer("ParticlesBuffer", particles_b, 0);
                particle_step.bind_buffer("ParticlesBufferUpdated", particles_a, 1);
            }
            is_particles_a = !is_particles_a;

            particle_step.use();

            particle_step.bind_uniform("view_width", (float) view_width);
            particle_step.bind_uniform("view_height", (float) view_height);
            particle_step.bind_uniform("internal_width", (float) internal_width);
            particle_step.bind_uniform("internal_height", (float) internal_height);
            particle_step.bind_uniform("w_k", w_k);
            particle_step.bind_uniform("mu_k", mu_k);
            particle_step.bind_uniform("sigma_k2", sigma_k2);
            particle_step.bind_uniform("mu_g", mu_g);
            particle_step.bind_uniform("sigma_g2", sigma_g2);
            particle_step.bind_uniform("c_rep", c_rep);
            particle_step.bind_uniform("r_distance", r_distance);
            particle_step.bind_uniform("num_particles", num_particles);
            particle_step.bind_uniform("h", h);
            particle_step.bind_uniform("h2", h2);
            particle_step.bind_uniform("dt", dt);
            particle_step.bind_uniform("translate_x", translate_x);
            particle_step.bind_uniform("translate_y", translate_y);

            particle_step.dispatch(num_particles, 1, 1);
            particle_step.wait();
        }
    }

    void display() {
        if (is_particles_a) {
            info_shader.bind_buffer("ParticlesBuffer", particles_a, 0);
        } else {
            info_shader.bind_buffer("ParticlesBuffer", particles_b, 0);
        }

        info_shader.use();
        info_shader.bind_uniform("view_width", (float) view_width);
        info_shader.bind_uniform("view_height", (float) view_height);
        info_shader.bind_uniform("internal_width", (float) internal_width);
        info_shader.bind_uniform("internal_height", (float) internal_height);
        info_shader.bind_uniform("w_k", w_k);
        info_shader.bind_uniform("mu_k", mu_k);
        info_shader.bind_uniform("sigma_k2", sigma_k2);
        info_shader.bind_uniform("mu_g", mu_g);
        info_shader.bind_uniform("sigma_g2", sigma_g2);
        info_shader.bind_uniform("c_rep", c_rep);
        info_shader.bind_uniform("r_distance", r_distance);
        info_shader.bind_uniform("num_particles", num_particles);
        info_shader.bind_uniform("h", h);
        info_shader.bind_uniform("h2", h2);
        info_shader.bind_uniform("dt", dt);
        info_shader.bind_uniform("render_1", render_1);
        info_shader.bind_uniform("render_2", render_2);
        info_shader.bind_uniform("background_color",
                                 std::array<float, 4>{background_color.x, background_color.y, background_color.z,
                                                      background_color.w});
        info_shader.bind_uniform("color1", std::array<float, 4>{color_1.x, color_1.y, color_1.z, color_1.w});
        info_shader.bind_uniform("color2", std::array<float, 4>{color_2.x, color_2.y, color_2.z, color_2.w});
        info_shader.bind_uniform("translate_x", translate_x);
        info_shader.bind_uniform("translate_y", translate_y);
        info_shader.render_to_window();
    }
};
