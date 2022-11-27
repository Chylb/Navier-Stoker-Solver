#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/ext/vector_float3.hpp>
#include <string>

enum class Tool {
	NONE,
	PRESSURE_POSITIVE,
	PRESSURE_NEGATIVE
};

enum class BoundaryCondition {
	FREE_SLIP =0,
	NO_SLIP = 1,
	OUTFLOW = 2,
	PERIODIC = 3
};

struct ImGuiParameterState
{
	int selected_radio;
};

static ImGuiParameterState state;

class Gui
{
public:
	static void Init(GLFWwindow* window);
	static void Terminate();
	static void RenderWindow(GLFWwindow* window);
	static BoundaryCondition getBoundary_condition();
	inline static Tool s_tool = Tool::NONE;

private:
	inline static bool s_showDemoWindow = false;
	inline static bool s_showModelParameters = false;
	inline static bool s_showAdditionalInfo = false;
	inline static BoundaryCondition s_boundary_condition = BoundaryCondition::FREE_SLIP;
	inline static std::string value = "uh";
	inline static const char* boundaryNames[4] = { "FREE SLIP", "NO_SLIP", "OUTFLOW", "PERIODIC" };

};