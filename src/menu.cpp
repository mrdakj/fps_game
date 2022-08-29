#include "menu.h"
#include <imgui.h>
#include <imgui_internal.h>

Menu::Menu(GLFWwindow *window, unsigned int window_width,
           unsigned int window_height)
    : m_width(window_width),
      m_height(window_height), m_state{GameState::NotStarted, 0, 0} {
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

Menu::Result Menu::draw_menu() const {
  assert(m_state.game_state != GameState::Running && "game not running");

  Menu::Result result = Menu::Result::None;

  int font_size = ImGui::GetFontSize();
  ImGui::SetNextWindowSize(ImVec2(20 * font_size, 20 * font_size));
  ImGui::SetNextWindowPos(ImVec2(m_width * 0.5f, m_height * 0.5f),
                          ImGuiCond_Always, ImVec2(0.5f, 0.5f));

  ImGui::Begin("FPS Game");
  spacing(0, 20);
  text_aligned(m_state.game_state == GameState::Over ? "GAME OVER" : "WELCOME");
  spacing(0, 20);
  float buttons_width = std::max(button_size("play"), button_size("exit"));
  if (button_aligned("play", buttons_width)) {
    result = Menu::Result::Play;
  }
  if (button_aligned("exit", buttons_width)) {
    result = Menu::Result::Exit;
  }
  ImGui::End();

  return result;
}

void Menu::draw_text() const {
  // hide cursor because game is running
  ImGui::SetMouseCursor(ImGuiMouseCursor_None);
  ImGui::GetForegroundDrawList()->AddText(
      ImVec2(0, 0), ImGui::ColorConvertFloat4ToU32({1, 1, 1, 1}),
      ("lives: " + std::to_string(m_state.lives) +
       "  bullets: " + std::to_string(m_state.bullets) +
       "  frame rate: " + std::to_string(m_state.frame_rate))
          .c_str());
}

Menu::Result Menu::update(State state) {
  m_state = std::move(state);
  Menu::Result result = Menu::Result::None;

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  switch (m_state.game_state) {
  case GameState::NotStarted:
  case GameState::Over: {
    result = draw_menu();
    break;
  }
  case GameState::Running: {
    draw_text();
    break;
  }
  default:
    assert(false && "unknown game state");
  }

  return result;
}

void Menu::spacing(float space_x, float space_y) const {
  ImGui::Dummy(ImVec2(space_x, space_y));
}

float Menu::label_size(const char *label) const {
  return ImGui::CalcTextSize(label).x;
}

float Menu::button_size(const char *label) const {
  return ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

void Menu::align(const char *label, float element_size) const {
  float off = (ImGui::GetContentRegionAvail().x - element_size) * 0.5;
  if (off > 0.0f) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
  }
}

void Menu::text_aligned(const char *label) const {
  align(label, label_size(label));
  ImGui::Text(label);
}

bool Menu::button_aligned(const char *label) const {
  align(label, button_size(label));
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
