#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glm/ext/vector_float3.hpp>

enum class Tool {
	NONE,
	PRESSURE_POSITIVE,
	PRESSURE_NEGATIVE
};

class Gui
{
public:
	static void Init(GLFWwindow* window);
	static void Terminate();
	static void RenderWindow(GLFWwindow* window);

	inline static Tool s_tool = Tool::NONE;
private:
	inline static bool s_showDemoWindow = false;
	inline static bool s_showModelParameters = false;
	inline static bool s_showAdditionalInfo = false;
};