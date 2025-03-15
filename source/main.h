#pragma once

#include <windows.h>
#include <psapi.h>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>

#include "shader.h"
#include "Camera.h"
//#include "Chunk.h"
#include "World.h"

class main
{
public:
	static void processRendering(GLFWwindow* window, shader& mainShader, World& world);

	static void initializeGLFW(GLFWwindow*& window);
	static void initializeGLAD();
	static void framebuffer_size_callback(GLFWwindow* window, GLint width, GLint height);
	static void setupRenderingState();

	static void updateFPS();

	static void initializeImGui(GLFWwindow* window);
	static void renderImGui(GLFWwindow* window);
	static void cleanupImGui();
	static void cleanup(shader& mainShader);
	static void scroll_callback(GLFWwindow* window, GLdouble xoffset, GLdouble yoffset);
	static void mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn);
	static void mouseButtonCallback(GLFWwindow* window, GLint button, GLint action, GLint mods);
	static size_t getCurrentMemoryUsage();

	static void processInput(GLFWwindow* window);
};