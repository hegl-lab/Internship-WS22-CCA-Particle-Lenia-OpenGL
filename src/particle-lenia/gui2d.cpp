#include <cmath>
#include <GLFWAbstraction.h>
#include <random>
#include <chrono>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

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
static ImVec4 background_color = ImVec4(1 / 255., 23 / 255., 47 / 255., 1.0);
static ImVec4 color_1 = ImVec4(46 / 255., 134 / 255., 171 / 255., 1.0);
static ImVec4 color_2 = ImVec4(241.0 / 255., 143.0 / 255., 1.0 / 255., 1.0);
int render_1 = 1;
int render_2 = 3;

float translate_x;
float translate_y;

bool is_particles_a = true;
bool pause = false;

int steps_per_frame = 10;

Buffer particles_a(num_particles * 2, GL_SHADER_STORAGE_BUFFER);
Buffer particles_b(num_particles * 2, GL_SHADER_STORAGE_BUFFER);

FragmentOnlyShader info_shader("shaders/particle-lenia/2d/fields_2d.generated.frag");
SimpleComputeShader particle_step("shaders/particle-lenia/2d/particle_2d.generated.comp");

bool render_loop_call(GLFWwindow *window);

void call_after_glfw_init(GLFWwindow *window);

unsigned int VBO;

int main() {
    init<render_loop_call, call_after_glfw_init>(view_width, view_height, "Particle Lenia 2D");
}

auto start = std::chrono::steady_clock::now();
int frame = 0;

void reset_particles() {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<> distribution(-((double) internal_width) * 0.3, ((double) internal_width) * 0.3);
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

bool render_loop_call(GLFWwindow *window) {
    for (int i = 0; i < steps_per_frame; ++i) {
        if (is_particles_a) {
            particle_step.bind_buffer("ParticlesBuffer", particles_a, 0);
            info_shader.bind_buffer("ParticlesBuffer", particles_a, 0);
            particle_step.bind_buffer("ParticlesBufferUpdated", particles_b, 1);
            //particle_display.bind_buffer("ParticlesBufferUpdated", particles_b, 1);
        } else {
            particle_step.bind_buffer("ParticlesBuffer", particles_b, 0);
            info_shader.bind_buffer("ParticlesBuffer", particles_b, 0);
            particle_step.bind_buffer("ParticlesBufferUpdated", particles_a, 1);
            //particle_display.bind_buffer("ParticlesBufferUpdated", particles_a, 1);
        }
        if (!pause) is_particles_a = !is_particles_a;

        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    // create control window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static float aspect_ratio = internal_width / internal_height;
    {
        static bool keep_aspect_ratio = true;

        ImGui::Begin("Controls");
        ImGui::Text("Press 'A' and hover over the screen to add more particles.");
        ImGui::Text("Press & hold 'Middle Mouse Button' and move your mouse around to move the picture.");

        ImGui::Checkbox("Pause", &pause);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            reset_particles();
        }

        if (ImGui::SliderFloat("Width", &internal_width, 1.0f, 200.0f) && keep_aspect_ratio)
            internal_height = internal_width / aspect_ratio;
        if (ImGui::SliderFloat("Height", &internal_height, 1.0f, 200.0f) && keep_aspect_ratio)
            internal_width = internal_height * aspect_ratio;
        if (ImGui::Checkbox("Keep Aspect Ratio", &keep_aspect_ratio)) aspect_ratio = internal_width / internal_height;

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
            ImGui::SliderInt("Steps per frame", &steps_per_frame, 1, 1000);
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::Text("Application average %.3f ms/step", (1000.0f / ImGui::GetIO().Framerate) / steps_per_frame);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::IsKeyPressed(ImGuiKey_A, true)) {
        ++num_particles;
        resize_buffer(false, false, translate_mouse_position(true));
    }

    static ImVec2 source_position;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
        source_position = translate_mouse_position();
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        ImVec2 position = translate_mouse_position();

        translate_x -= position.x - source_position.x;
        translate_y -= position.y - source_position.y;

        source_position = position;
    }

    if (view_width != CURRENT_WIDTH || view_height != CURRENT_HEIGHT) {
        internal_width *= (float) CURRENT_WIDTH / (float) view_width;
        internal_height *= (float) CURRENT_HEIGHT / (float) view_height;

        view_width = CURRENT_WIDTH;
        view_height = CURRENT_HEIGHT;

        aspect_ratio = internal_width / internal_height;
    }

    return true;
}

void call_after_glfw_init(GLFWwindow *window) {
    /*glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(num_particles), nullptr, GL_STATIC_DRAW);*/


    particles_a.init();
    particles_b.init();

    glPointSize(2.1);

    // generate random particles
    reset_particles();

    info_shader.init_without_arguments();
    particle_step.init_without_arguments();

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
