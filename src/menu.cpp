#include "menu.h"

Menu::Menu(GLFWwindow *window, unsigned int window_width,
           unsigned int window_height)
    : m_width(window_width), m_height(window_height) {
  ImGui::CreateContext();

  ImGui::StyleColorsDark();
  const char *glsl_version = "#version 150";

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  int dpi_scale = 3;
  int font_size = 13;
  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF("../res/fonts/sf_atarian_system.ttf",
                               font_size * dpi_scale);
  ImGui::GetStyle().ScaleAllSizes(dpi_scale);
}

Menu::Result Menu::update(bool game_over) const {
  Menu::Result result = Menu::Result::None;

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  auto font_size = ImGui::GetFontSize();
  ImGui::SetNextWindowSize(ImVec2(20 * font_size, 20 * font_size));
  ImGui::SetNextWindowPos(ImVec2(m_width * 0.5f, m_height * 0.5f),
                          ImGuiCond_Always, ImVec2(0.5f, 0.5f));

  ImGui::Begin("FPS Game");
  spacing(0, 20);
  text_aligned(game_over ? "GAME OVER" : "WELCOME");
  spacing(0, 20);
  float buttons_width = std::max(label_size("play"), label_size("exit"));
  if (button_aligned("play", buttons_width)) {
    result = Menu::Result::Play;
  }
  if (button_aligned("exit", buttons_width)) {
    result = Menu::Result::Exit;
  }
  ImGui::End();

  return result;
}

void Menu::spacing(float space_x, float space_y) const {
  ImGui::Dummy(ImVec2(space_x, space_y));
}

float Menu::label_size(const char *label) const {
  ImGuiStyle &style = ImGui::GetStyle();
  return ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
}

void Menu::align(const char *label, float element_size) const {
  float avail = ImGui::GetContentRegionAvail().x;
  float alignment = 0.5f;

  float off = (avail - element_size) * alignment;
  if (off > 0.0f) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
  }
}

void Menu::text_aligned(const char *label) const {
  align(label, label_size(label));
  ImGui::Text(label);
}

bool Menu::button_aligned(const char *label) const {
  align(label, label_size(label));
  return ImGui::Button(label);
}

bool Menu::button_aligned(const char *label, float element_size) const {
  align(label, element_size);
  return ImGui::Button(label, ImVec2(element_size, 0));
}

void Menu::render() const {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

Menu::~Menu() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
