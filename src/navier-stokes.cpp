#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <thread>

#include "Shader.h"


#include <iostream>
#include <vector>

#include <Eigen/Dense>
#include <corecrt_math_defines.h>

using namespace Eigen;

static const unsigned int SCR_WIDTH = 800, SCR_HEIGHT = 600;

#define REAL float

int it_max = 150;
constexpr int nx = 40;
constexpr int ny = 40;
constexpr int nx1 = nx + 1;
constexpr int ny1 = ny + 1;

int width = nx + 2;
int height = nx + 2;

REAL dx = 1;
REAL dy = 1;
REAL tau = 0.5;
REAL dt = 0;
REAL alpha = 0.9;
REAL omega = 1;
REAL t = 0;
int n = 0;

REAL re = 1;
REAL epsilon = 1e-5;
Array<REAL, nx + 2, ny + 2> U, V, P, RHS, F, G;
auto Uc = U.block(1, 1, nx, ny);
auto Vc = V.block(1, 1, nx, ny);
auto Pc = P.block(1, 1, nx, ny);
auto Fc = F.block(1, 1, nx, ny);
auto Gc = G.block(1, 1, nx, ny);
auto RHSc = RHS.block(1, 1, nx, ny);

int doSimulationStep();
void computeDeltaT();
void setBoundaryConditions();
void setSpecificBoundaryConditions();
void computeFG();
void computeRightHandSide();
void adaptUV();
REAL SORPoisson();

GLFWwindow* initGLFW();
GLuint buildArrow();
GLuint buildPressureTexture();
GLuint buildPressureQuad();
std::pair<GLuint, unsigned int> buildVelocity(GLuint arrowVAO);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

int main()
{
	GLFWwindow* window = initGLFW();
	if (!window) {
		return -1;
	}

	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	GLuint arrowVAO = buildArrow();

	Shader texShader("res/tex.vs", "res/tex.fs");
	Shader arrowShader("res/arrow.vs", "res/arrow.fs");

	int width = 42, height = 42;

	unsigned int pressureQuadVAO = buildPressureQuad();

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	while (!glfwWindowShouldClose(window)) {
		int numPressureIterations = doSimulationStep();
		t += dt;
		n++;

		//begin
		glClearColor(0.2f, 0.6f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		GLuint texture = buildPressureTexture();
		auto [buffer, arrowCount] = buildVelocity(arrowVAO);

		//draw
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		texShader.use();
		glBindVertexArray(pressureQuadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		arrowShader.use();
		glBindVertexArray(arrowVAO);
		glDrawArraysInstanced(GL_TRIANGLES, 0,9, arrowCount);

		//delete
		glDeleteTextures(1, &texture);
		glDeleteBuffers(1, &buffer);

		//end
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}

int doSimulationStep()
{
	computeDeltaT();
	setBoundaryConditions();
	setSpecificBoundaryConditions();
	computeFG();
	computeRightHandSide();

	REAL residual = INFINITY;

	int sor_iterations = 0;
	for (; sor_iterations < it_max && fabs(residual) > epsilon; ++sor_iterations)
	{
		residual = SORPoisson();
	}

	adaptUV();

	return sor_iterations;
}

void computeDeltaT()
{
	REAL u_max = 0.0, v_max = 0.0;
	REAL opt_a, opt_x, opt_y, min;

	for (int y = 1; y < ny1; ++y)
	{
		for (int x = 1; x < nx1; ++x)
		{
			if (fabs(U(y, x)) > u_max)
				u_max = fabs(U(y, x));
			if (fabs(V(y, x)) > v_max)
				v_max = fabs(V(y, x));
		}
	}

	opt_a = (re / 2.0)
		* 1.0 / (
			1.0 / (dx * dx)
			+ 1.0 / (dy * dy)
			);
	opt_x = dx / fabs(u_max);
	opt_y = dy / fabs(v_max);

	min = opt_a < opt_x ? opt_a : opt_x;
	min = min < opt_y ? min : opt_y;

	dt = tau * min;
}

void setBoundaryConditions()
{
	// something maybe wrong with 
	U.topRows(1) = U.block(1, 0, 1, ny);
	V.topRows(1) = 0;
	U.bottomRows(1) = U.block(nx, 0, 1, ny);
	V.bottomRows(1) = 0;

	U.leftCols(1) = 0;
	V.leftCols(1) = V.block(0, 1, nx, 1);
	U.rightCols(1) = 0;
	V.rightCols(1) = V.block(0, ny, nx, 1);
}

void setSpecificBoundaryConditions()
{
	U.topRows(1) = 1;
	P.topRows(1) = 1; //todo 0 or 1
	//P.bottomRows(1) = 0;
}

void computeFG()
{
	auto d2m_dx2 = (U.block(1, 0, nx, ny) - 2 * Uc + U.block(1, 2, nx, ny)) / dx / dx;
	auto d2m_dy2 = (U.block(0, 1, nx, ny) - 2 * Uc + U.block(1, 2, nx, ny)) / dy / dy;
	auto dv2_dy = (
		(
			(Vc + V.block(2, 1, nx, ny)) * (Vc + V.block(2, 1, nx, ny)) -
			(V.block(0, 1, nx, ny) + Vc) * (V.block(0, 1, nx, ny) + Vc)
			)
		+ alpha * (
			(Vc + V.block(2, 1, nx, ny)).cwiseAbs() * (Vc - V.block(2, 1, nx, ny)) -
			(Vc + V.block(0, 1, nx, ny)).cwiseAbs() * (Vc - V.block(0, 1, nx, ny))
			)) / (4 * dy);

	auto du2_dx = (
		(
			(Uc + U.block(1, 2, nx, ny)) * (Uc + U.block(1, 2, nx, ny)) -
			(U.block(1, 0, nx, ny) + Uc) * (U.block(1, 0, nx, ny) + Uc)
			)
		+ alpha * (
			(Uc + U.block(1, 2, nx, ny)).cwiseAbs() * (Uc - U.block(1, 2, nx, ny)) -
			(Uc + U.block(1, 0, nx, ny)).cwiseAbs() * (Uc - U.block(1, 0, nx, ny))
			)) / (4 * dx);

	auto duv_dx = (
		(
			(Uc + U.block(2, 1, nx, ny) * (Vc + V.block(1, 2, nx, ny))) -
			(U.block(1, 0, nx, ny) + U.block(2, 0, nx, ny)) * (V.block(1, 0, nx, ny) + Vc)
			)
		+ alpha * (
			(Uc + U.block(2, 1, nx, ny)).cwiseAbs() * (Vc - V.block(1, 2, nx, ny)) -
			(U.block(1, 0, nx, ny) + U.block(2, 0, nx, ny)).cwiseAbs() * (V.block(1, 0, nx, ny) - Vc)
			)) / (4 * dx);

	auto duv_dy = (
		(
			(Vc + V.block(1, 2, nx, ny) * (Uc + U.block(2, 1, nx, ny))) -
			(V.block(0, 1, nx, ny) + V.block(0, 2, nx, ny)) * (U.block(0, 1, nx, ny) + Uc)
			)
		+ alpha * (
			(Vc + V.block(1, 2, nx, ny)).cwiseAbs() * (Uc - U.block(2, 1, nx, ny)) -
			(V.block(0, 1, nx, ny) + V.block(0, 2, nx, ny)).cwiseAbs() * (U.block(0, 1, nx, ny) - Uc)
			)) / (4 * dy);


	Fc = Uc + dt * (
		(d2m_dx2 + d2m_dy2) / re
		- du2_dx - duv_dy
		);

	Gc = Vc + dt * (
		(d2m_dx2 + d2m_dy2) / re
		- dv2_dy - duv_dx
		);

	F.leftCols(1) = U.leftCols(1);
	F.rightCols(1) = U.rightCols(1);

	G.topRows(1) = V.topRows(1);
	G.bottomRows(1) = V.bottomRows(1);
}

void computeRightHandSide()
{
	RHSc = 1 / dt * (
		(Fc - F.block(1, 0, nx, ny)) / dx +
		(Gc - G.block(0, 1, nx, ny)) / dy
		);
}

void adaptUV()
{
	//TODO SECOND LOOP x < nx?
	Uc = Fc - (dt / dx) * (P.block(1, 2, nx, ny) - Pc);
	Vc = Gc - (dt / dy) * (P.block(2, 1, nx, ny) - Pc);

}

REAL SORPoisson()
{
	REAL dx2 = dx * dx;
	REAL dy2 = dy * dy;

	REAL constant_expr = omega / (2.0 / dx2 + 2.0 / dy2);

	Pc = (1 - omega) * Pc +
		constant_expr * (
			(P.block(1, 0, nx, ny) + P.block(1, 2, nx, ny)) / dx2 +
			(P.block(0, 1, nx, ny) + P.block(2, 1, nx, ny)) / dy2 -
			RHSc
			);

	P.topRows(1) = P.block(1, 0, 1, ny);
	P.bottomRows(1) = P.block(nx, 0, 1, ny);
	P.leftCols(1) = P.block(0, 1, nx, 1);
	P.rightCols(1) = P.block(0, ny, nx, 1);

	auto tmp = ((P.block(1, 2, nx, ny) - Pc) - (Pc - P.block(1, 0, nx, ny))) / dx2 +
		((P.block(2, 1, nx, ny) - Pc) - (Pc - P.block(0, 1, nx, ny))) / dy2 -
		RHSc;

	int numCells = nx * ny;

	auto sum = (tmp * tmp).sum();
	return sqrt(sum / numCells);
}

GLuint buildArrow() {

	float width = 0.2;
	float head_bottom = 0.3;
	float vertices[] = {
			-0.5,head_bottom,0,
			0,1,0,
			0.5,head_bottom,0,

			-width,-1,0,
			-width,0.5,0,
			width,-1,0,

			width,-1,0,
			width,0.5,0,
			-width,0.5,0
	};

	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return VAO;
}

GLFWwindow* initGLFW() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	auto window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Navier-Stokes simulation", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//glfwSetCursorPosCallback(window, mouse_callback);
	//glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialize GLEW!\n";
		return nullptr;
	}
	return window;
}

GLuint buildPressureTexture() {
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	int width = 42, height = 42;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, (void*)P.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texture;
}

GLuint buildPressureQuad() {

	float vertices[] = {
		-1, -1, 0.0f, 0, 0,
		 1, -1, 0.0f, 1.0f, 0.0f,
		 -1,  1, 0.0f, 0.0f, 1,

		 -1,  1, 0.0f, 0.0f, 1,
		 1, -1, 0.0f, 1.0f, 0.0f,
		 1,1,0,1,1
	};

	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

std::pair<GLuint, unsigned int>  buildVelocity(GLuint arrowVAO) {
	std::vector<glm::mat4> modelMatrices{ 400 };
	for (int x = 0; x < width; x += 2)
		for (int y = 0; y < width; y += 2) {
			auto u = U(x, y);
			auto v = V(x, y);
			auto len = sqrt(u * u + v * v);
			auto ang = acos((u / len));
			auto sizeConst = 0.05f;

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(-1 + 2 * x / (float)width, -1 + 2 * y / (float)height, 0.0f));
			model = glm::scale(model, { sizeConst * len, sizeConst * len,sizeConst * len });
			model = glm::rotate(model, ang, glm::vec3(0.f, 0.f, 1.f));
			model = glm::translate(model, { 0,1,0 });
			modelMatrices.push_back(model);
		}
	unsigned int buffer;
	glGenBuffers(1, &buffer);
	glBindVertexArray(arrowVAO);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

	std::size_t vec4Size = sizeof(glm::vec4);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return { buffer, modelMatrices.size() };
}
