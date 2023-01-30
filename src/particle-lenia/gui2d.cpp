#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "particle_lenia_2d.hpp"

bool pause = false;
int steps_per_frame = 10;

ParticleLenia2D particle_lenia;

bool render_loop_call(GLFWwindow *window);

void call_after_glfw_init(GLFWwindow *window);

int main() {
    init<render_loop_call, call_after_glfw_init>(particle_lenia.view_width, particle_lenia.view_height,
                                                 "Particle Lenia 2D");
}

auto start = std::chrono::steady_clock::now();
int frame = 0;

bool render_loop_call(GLFWwindow *window) {
    particle_lenia.step(steps_per_frame);
    particle_lenia.display();

    // create control window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static float aspect_ratio = particle_lenia.internal_width / particle_lenia.internal_height;
    {
        static bool keep_aspect_ratio = true;

        ImGui::Begin("Controls");
        ImGui::Text("Press 'A' and hover over the screen to add more particles.");
        ImGui::Text("Press & hold 'Middle Mouse Button' and move your mouse around to move the picture.");

        ImGui::Checkbox("Pause", &pause);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            particle_lenia.reset_particles();
        }

        if (ImGui::SliderFloat("Width", &particle_lenia.internal_width, 1.0f, 200.0f) && keep_aspect_ratio)
            particle_lenia.internal_height = particle_lenia.internal_width / aspect_ratio;
        if (ImGui::SliderFloat("Height", &particle_lenia.internal_height, 1.0f, 200.0f) && keep_aspect_ratio)
            particle_lenia.internal_width = particle_lenia.internal_height * aspect_ratio;
        if (ImGui::Checkbox("Keep Aspect Ratio", &keep_aspect_ratio))
            aspect_ratio = particle_lenia.internal_width /
                           particle_lenia.internal_height;

        {
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            const char *items[] = {"None", "U", "Repulsion", "Growth", "Energy (abs)"};
            ImGui::ColorEdit3("Color 1", (float *) &particle_lenia.color_1);
            ImGui::SameLine();
            ImGui::Combo("Render 1", &particle_lenia.render_1, items, IM_ARRAYSIZE(items));
            ImGui::ColorEdit3("Color 2", (float *) &particle_lenia.color_2);
            ImGui::SameLine();
            ImGui::Combo("Render 2", &particle_lenia.render_2, items, IM_ARRAYSIZE(items));
            ImGui::SetColorEditOptions(ImGuiColorEditFlags_NoInputs);
            ImGui::ColorEdit3("Background Color", (float *) &particle_lenia.background_color);
        }

        ImGui::NewLine();
        ImGui::Text("Field settings");
        static bool reset_on_change = false;
        ImGui::Checkbox("Reset particles on settings change", &reset_on_change);

        {
            ImGui::Text("Kernel settings");
            if (ImGui::SliderFloat("w_k", &particle_lenia.w_k, 0.0f, 1.f) && reset_on_change)
                particle_lenia.reset_particles();
            if (ImGui::SliderFloat("mu_k", &particle_lenia.mu_k, 0.0f, 20.f) && reset_on_change)
                particle_lenia.reset_particles();
            if (ImGui::SliderFloat("sigma_k^2", &particle_lenia.sigma_k2, 0.0f, 10.f) && reset_on_change)
                particle_lenia.reset_particles();
        }
        {
            ImGui::Text("Growth settings");
            if (ImGui::SliderFloat("mu_g", &particle_lenia.mu_g, 0.0f, 10.f) && reset_on_change)
                particle_lenia.reset_particles();
            if (ImGui::SliderFloat("sigma_g^2", &particle_lenia.sigma_g2, 0.0f, 3.f) && reset_on_change)
                particle_lenia.reset_particles();
        }
        {
            ImGui::Text("Repulsion settings");
            if (ImGui::SliderFloat("c_rep", &particle_lenia.c_rep, 0.f, 10.f) && reset_on_change)
                particle_lenia.reset_particles();
            // if (ImGui::SliderFloat("r_distance", &r_distance, 0.f, 1e-8) && reset_on_change) particle_lenia.reset_particles();
        }
        {
            ImGui::Text("Misc");
            if (ImGui::SliderFloat("h (gradient evaluation distance)", &particle_lenia.h, 0.f, 0.1f)) {
                particle_lenia.h2 = 2 * particle_lenia.h;
                if (reset_on_change) particle_lenia.reset_particles();
            }
            if (ImGui::SliderFloat("dt", &particle_lenia.dt, 0.f, 3.f) && reset_on_change)
                particle_lenia.reset_particles();
            if (ImGui::SliderInt("Number of Particles", &particle_lenia.num_particles, 0, 2500)) {
                particle_lenia.resize_buffer(reset_on_change);
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
        ++particle_lenia.num_particles;
        particle_lenia.resize_buffer(false, false, particle_lenia.translate_mouse_position(true));
    }

    static ImVec2 source_position;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
        source_position = particle_lenia.translate_mouse_position();
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        ImVec2 position = particle_lenia.translate_mouse_position();

        particle_lenia.translate_x -= position.x - source_position.x;
        particle_lenia.translate_y -= position.y - source_position.y;

        source_position = position;
    }

    if (particle_lenia.view_width != CURRENT_WIDTH || particle_lenia.view_height != CURRENT_HEIGHT) {
        particle_lenia.internal_width *= (float) CURRENT_WIDTH / (float) particle_lenia.view_width;
        particle_lenia.internal_height *= (float) CURRENT_HEIGHT / (float) particle_lenia.view_height;

        particle_lenia.view_width = CURRENT_WIDTH;
        particle_lenia.view_height = CURRENT_HEIGHT;

        aspect_ratio = particle_lenia.internal_width / particle_lenia.internal_height;
    }

    return true;
}

void call_after_glfw_init(GLFWwindow *window) {
    particle_lenia.init();

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
