#include <cmath>
#include <GLFWAbstraction.h>
#include <random>
#include <chrono>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <GL/gl.h>

std::array<float, 3> scale{1. / 10, 1. / 10, 1. / 10};
std::array<float, 9> rotate{
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
};
float angle1 = 0;
std::array<float, 3> translate{0, 0, 0};

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
static ImVec4 background_color = ImVec4(1 / 255., 23 / 255., 47 / 255., 1.0);
static ImVec4 color_1 = ImVec4(46 / 255., 134 / 255., 171 / 255., 1.0);
static ImVec4 color_2 = ImVec4(241.0 / 255., 143.0 / 255., 1.0 / 255., 1.0);
int render_1 = 1;
int render_2 = 3;
float depth = 3.;

bool is_particles_a = true;
bool pause = true;

unsigned int VAO;

Buffer particles_a(num_particles * 3, GL_SHADER_STORAGE_BUFFER);
Buffer particles_b(num_particles * 3, GL_SHADER_STORAGE_BUFFER);

SimpleShader shader("shaders/particle-lenia/3d/particle_3d.vert", "shaders/particle-lenia/3d/particle_3d.frag");
FragmentOnlyShader info_shader("shaders/particle-lenia/3d/fields_3d.frag");

bool render_loop_call(GLFWwindow *window);

void call_after_glfw_init(GLFWwindow *window);

unsigned int VBO;

int main() {
    init<render_loop_call, call_after_glfw_init>(view_width, view_height, "Particle Lenia 3D");
}

auto start = std::chrono::steady_clock::now();
int frame = 0;

void reset_particles() {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> distribution(-10, 10);
    std::vector<float> particles;
    for (int i = 0; i < 3 * num_particles; ++i) {
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
        std::uniform_real_distribution<> distribution(-10, 10);
        for (int i = 0; i < 3 * num_particles; ++i) {
            data.emplace_back(distribution(rng));
        }
    } else {
        data = particles_a.get_data();

        // create more data if necessary
        if (data.size() < num_particles * 3) {
            if (append_random) {
                std::random_device dev;
                std::mt19937 rng(dev());
                std::uniform_real_distribution<> distribution(-10, 10);
                while (data.size() < num_particles * 3) data.push_back(distribution(rng));
            } else {
                while (data.size() < num_particles * 3) {
                    data.push_back(append.x);
                    data.push_back(append.y);
                }
            }
        } else {
            data.resize(3 * num_particles);
        }
    }

    particles_a.delete_buffer();
    particles_b.delete_buffer();

    particles_a = Buffer(3 * num_particles, GL_SHADER_STORAGE_BUFFER);
    particles_b = Buffer(3 * num_particles, GL_SHADER_STORAGE_BUFFER);

    particles_a.init();
    particles_b.init();

    particles_a.set_data(data);
    particles_b.set_data(data);
}

// currently deprecated
ImVec2 translate_mouse_position(bool include_translate = false) {
    ImVec2 position = ImGui::GetMousePos();
    return position;
}

bool render_loop_call(GLFWwindow *window) {
    if (is_particles_a) {
        shader.bind_buffer("ParticlesBuffer", particles_a, 0);
        info_shader.bind_buffer("ParticlesBuffer", particles_a, 0);
        shader.bind_buffer("ParticlesBufferUpdated", particles_b, 1);
    } else {
        shader.bind_buffer("ParticlesBuffer", particles_b, 0);
        info_shader.bind_buffer("ParticlesBuffer", particles_b, 0);
        shader.bind_buffer("ParticlesBufferUpdated", particles_a, 1);
    }
    if (!pause) is_particles_a = !is_particles_a;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    info_shader.use();
    info_shader.bind_uniform("view_width", (float) view_width);
    info_shader.bind_uniform("view_height", (float) view_height);
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
    info_shader.bind_uniform("rotation", rotate);
    info_shader.bind_uniform("translate", translate);
    info_shader.bind_uniform("scale", scale);
    info_shader.bind_uniform("depth", depth);
    info_shader.render_to_window();

    shader.use();

    shader.bind_uniform("view_width", (float) view_width);
    shader.bind_uniform("view_height", (float) view_height);
    shader.bind_uniform("w_k", w_k);
    shader.bind_uniform("mu_k", mu_k);
    shader.bind_uniform("sigma_k2", sigma_k2);
    shader.bind_uniform("mu_g", mu_g);
    shader.bind_uniform("sigma_g2", sigma_g2);
    shader.bind_uniform("c_rep", c_rep);
    shader.bind_uniform("r_distance", r_distance);
    shader.bind_uniform("num_particles", num_particles);
    shader.bind_uniform("h", h);
    shader.bind_uniform("h2", h2);
    shader.bind_uniform("dt", dt);
    shader.bind_uniform("rotation", rotate);
    shader.bind_uniform("translate", translate);
    shader.bind_uniform("scale", scale);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, num_particles);


    // create control window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Controls");
        ImGui::Text("Press 'A' and hover over the screen to add more particles.");
        ImGui::Text("Press & hold 'Middle Mouse Button' and move your mouse around to move the picture.");
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", translate[0], translate[1], translate[2]);

        ImGui::Checkbox("Pause", &pause);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            reset_particles();
        }

        ImGui::SliderFloat("Depth", &depth, 0, 20);

        {
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            const char *items[] = {"None", "U", "Repulsion", "Growth", "Energy (abs)"};
            ImGui::ColorEdit3("Color 1", (float *) &color_1);
            ImGui::SameLine();
            ImGui::Combo("Render 1", &render_1, items, IM_ARRAYSIZE(items));
            ImGui::ColorEdit3("Color 2", (float *) &color_2);
            ImGui::SameLine();
            ImGui::Combo("Render 2", &render_2, items, IM_ARRAYSIZE(items));
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Background Color", (float *) &background_color);
        }

        ImGui::NewLine();
        ImGui::Text("Field settings");
        static bool reset_on_change = false;
        ImGui::Checkbox("Reset particles on settings change", &reset_on_change);

        {
            ImGui::Text("Kernel settings");
            if (ImGui::SliderFloat("w_k", &w_k, 0.0f, 1.f) && reset_on_change) reset_particles();
            if (ImGui::SliderFloat("mu_k", &mu_k, 0.0f, 20.f) && reset_on_change) reset_particles();
            if (ImGui::SliderFloat("sigma_k^2", &sigma_k2, 0.0f, 10.f) && reset_on_change) reset_particles();
        }
        {
            ImGui::Text("Growth settings");
            if (ImGui::SliderFloat("mu_g", &mu_g, 0.0f, 10.f) && reset_on_change) reset_particles();
            if (ImGui::SliderFloat("sigma_g^2", &sigma_g2, 0.0f, 3.f) && reset_on_change) reset_particles();
        }
        {
            ImGui::Text("Repulsion settings");
            if (ImGui::SliderFloat("c_rep", &c_rep, 0.f, 10.f) && reset_on_change) reset_particles();
            // if (ImGui::SliderFloat("r_distance", &r_distance, 0.f, 1e-8) && reset_on_change) reset_particles();
        }
        {
            ImGui::Text("Misc");
            if (ImGui::SliderFloat("h (gradient evaluation distance)", &h, 0.f, 0.1f)) {
                h2 = 2 * h;
                if (reset_on_change) reset_particles();
            }
            if (ImGui::SliderFloat("dt", &dt, 0.f, 3.f) && reset_on_change) reset_particles();
            if (ImGui::SliderInt("Number of Particles", &num_particles, 0, 2500)) {
                resize_buffer(reset_on_change);
            }
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::End();
    }

    static double time = ImGui::GetTime();
    double current_time = ImGui::GetTime();
    double dt = time - current_time;
    time = current_time;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (ImGui::IsKeyPressed(ImGuiKey_W, true)) {
        translate[2] -= 1000. * dt;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_S, true)) {
        translate[2] += 1000. * dt;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_A, true)) {
        translate[0] += 1000. * dt;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_D, true)) {
        translate[0] -= 1000. * dt;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Space, true)) {
        translate[1] -= 1000. * dt;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_LeftShift, true)) {
        translate[1] += 1000. * dt;
    }


    static ImVec2 source_position;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
        source_position = translate_mouse_position();
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        ImVec2 position = translate_mouse_position();

        // TODO: implement move

        source_position = position;
    }

    if (view_width != CURRENT_WIDTH || view_height != CURRENT_HEIGHT) {
        view_width = CURRENT_WIDTH;
        view_height = CURRENT_HEIGHT;
    }

    return true;
}

void call_after_glfw_init(GLFWwindow *window) {
    particles_a.init();
    particles_b.init();

    //glPointSize(2.1);

    // generate random particles
    reset_particles();

    shader.init_without_arguments();
    info_shader.init_without_arguments();

    glGenVertexArrays(1, &VAO);

    // ImGui setup following
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}
