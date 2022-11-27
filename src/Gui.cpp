#include "Gui.h"

#include "imgui/imgui_internal.h"

#include <Eigen/Dense>

using namespace Eigen;

#define REAL float
extern Array<REAL, 42, 42> U, V, P, RHS, F, G;
extern REAL dt;
extern REAL t;
extern int n;
extern int numPressureIterations;
extern float realUpdateDt;

extern int it_max;
extern REAL alpha;
extern REAL tau;
extern REAL omega;

template <typename Enumeration>
auto as_integer(Enumeration const value)
-> typename std::underlying_type<Enumeration>::type
{
	return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

BoundaryCondition Gui::getBoundary_condition() {
	return s_boundary_condition;
}

void Gui::Init(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 430";
	ImGui_ImplOpenGL3_Init(glsl_version);
}

void Gui::Terminate()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void Gui::RenderWindow(GLFWwindow* window)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowPos({ 0,0 });
	//ImGui::SetNextWindowSize({ 210,-1 });
	ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

	if (ImGui::Button("Pressure-")) {
		s_tool = Tool::PRESSURE_NEGATIVE;
	}
	ImGui::SameLine();
	if (ImGui::Button("Pressure+")) {
		s_tool = Tool::PRESSURE_POSITIVE;
	}

	if (ImGui::Button("Reset model")) {
		P = 0;
		U = 0;
		V = 0;
		t = 0;
		n = 0;
	}
	ImGui::SetNextItemWidth(130);
	ImGui::DragFloat("real dt", &realUpdateDt, 0.01f, 0, 1);

	ImGui::Text("dt: %.2f", dt);
	ImGui::Text("pressure iterations: %d", numPressureIterations);
	ImGui::Text("t: %.2f", t);
	ImGui::Text("frame: %d", n);

	ImGui::Checkbox("Show additional info", &s_showAdditionalInfo);
	if (s_showAdditionalInfo) {
		int elements = P.rows() * P.cols();
		ImGui::Text("P min max avg: %.1f %.1f %.1f", P.minCoeff(), P.sum() / elements, P.maxCoeff());
		ImGui::Text("U min max avg: %.1f %.1f %.1f", U.minCoeff(), U.sum() / elements, U.maxCoeff());
		ImGui::Text("V min max avg: %.1f %.1f %.1f", V.minCoeff(), V.sum() / elements, V.maxCoeff());
	}

	ImGui::Checkbox("Show model parameters", &s_showModelParameters);
	if (s_showModelParameters) {
		ImGui::PushItemWidth(100);
		ImGui::DragInt("Max P iters", &it_max, 1, 10, 1000);
		ImGui::DragFloat("tau", &tau, 0.01f, 0, 1);
		ImGui::DragFloat("alpha", &alpha, 0.01f, 0, 1);
		ImGui::DragFloat("omega", &omega, 0.01f, 0, 2);
		ImGui::PopItemWidth();
	}
	ImGui::Text("Choose boundary condtions: %s", boundaryNames[as_integer(s_boundary_condition)]);
	if (ImGui::BeginTable("9x9_radio_table", 2))
	{
		for (int i = 0; i < 4; i++)
		{
			ImGui::TableNextColumn();
			if (ImGui::RadioButton(boundaryNames[i], &state.selected_radio, i))
			{
				if (i == 0) {
					s_boundary_condition = BoundaryCondition::FREE_SLIP;
				}
				else if (i  == 1) {
					s_boundary_condition = BoundaryCondition::NO_SLIP;
				}
				else if (i == 2) {
					s_boundary_condition = BoundaryCondition::OUTFLOW;
				}
				else {
					s_boundary_condition = BoundaryCondition::PERIODIC;
				}
				//std::cout << row << ";" << col << std::endl;
			}
		}
		ImGui::EndTable();
	}


	/*ImGui::Checkbox("ImGui demo window", &s_showDemoWindow);
	if (s_showDemoWindow) {
		ImGui::ShowDemoWindow(&s_showDemoWindow);
	}*/

	if (ImGui::Button("Exit")) {
		glfwSetWindowShouldClose(window, true);
	}

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}
