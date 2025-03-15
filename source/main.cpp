#include "main.h"

#pragma region Global Variables
constexpr GLuint SCR_WIDTH = 1280;
constexpr GLuint SCR_HEIGHT = 720;

GLfloat lastX = SCR_WIDTH / 2.0f;
GLfloat lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

Camera camera;

GLdouble lastTime = glfwGetTime();
uint8_t nbFrames = 0;
GLfloat fps = 0;

bool isGUIEnabled = false;
bool escapeKeyPressedLastFrame = false;

std::vector<GLfloat> memoryUsageHistory;
constexpr int8_t MEMORY_HISTORY_SIZE = 100;
#pragma endregion

int main()
{
	GLFWwindow* window;
	main::initializeGLFW(window);
	main::initializeGLAD();

	glfwSetFramebufferSizeCallback(window, main::framebuffer_size_callback);
	glfwSetCursorPosCallback(window, main::mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	shader mainShader("main.vs", "main.fs");

	glfwSetScrollCallback(window, main::scroll_callback);
	glfwSetMouseButtonCallback(window, main::mouseButtonCallback);

	//Chunk chunk;
	World world;

	main::setupRenderingState();

	main::initializeImGui(window);
	

	// -- Main Game Loop -- //
	while (!glfwWindowShouldClose(window))
	{
		main::processInput(window);
		main::updateFPS();
		camera.update(deltaTime);

		main::processRendering(window, mainShader, world);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	main::cleanupImGui();
	main::cleanup(mainShader);

	glfwTerminate();
	return 0;
}

void main::processRendering(GLFWwindow* window, shader& mainShader, World& world)
{
	// Prepare matrices
	glm::mat4 view = camera.getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(75.0f), (GLfloat)(SCR_WIDTH / (GLfloat)SCR_HEIGHT), 0.1f, 320.0f);
	glm::mat4 model = glm::mat4(1.0f);

	// Clear Buffers
	glClearColor(0.4f, 0.6f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set Shader Uniforms
	mainShader.use();
	mainShader.setMat4("model", model);
	mainShader.setMat4("view", view);
	mainShader.setMat4("projection", projection);

	mainShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));
	mainShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
	mainShader.setVec3("lightPos", glm::vec3(5.0f, 80.0f, 5.0f));

	//chunk.render(mainShader);
	world.render(mainShader);

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// ImGui
	if (isGUIEnabled) main::renderImGui(window);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void main::initializeGLFW(GLFWwindow*& window)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VOXLY XDDDDD", nullptr, nullptr);

	if (!window)
	{
		std::cerr << "Failed to create GLFW window!:(" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, GLint width, GLint height) {
		glViewport(0, 0, width, height);
		});
}

void main::initializeGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD!" << std::endl;
		glfwTerminate();
		exit(-1);
	}
}

void main::framebuffer_size_callback(GLFWwindow* window, GLint width, GLint height)
{
	glViewport(0, 0, width, height);
}

void main::setupRenderingState()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);
}

void main::updateFPS()
{
	GLfloat currentTime = static_cast<GLfloat>(glfwGetTime());
	nbFrames++;
	if (currentTime - lastTime >= 1.0) {
		fps = nbFrames;
		nbFrames = 0;
		lastTime += 1.0;
	}
	deltaTime = currentTime - lastFrame;
	lastFrame = currentTime;
}

void main::initializeImGui(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");
	ImGui::StyleColorsDark();
}

void main::renderImGui(GLFWwindow* window)
{
	glDisable(GL_DEPTH_TEST);

	ImGui::Begin("Menu");

	ImGui::Text("FPS: %.1f", fps); // FPS counter

	//// Memory Usage Graph ////
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Memory Usage", ImGuiTreeNodeFlags_DefaultOpen)) {
		// Get current memory usage in MB
		size_t memoryUsage = main::getCurrentMemoryUsage() / (1024 * 1024);

		// Add current memory usage to history
		if (memoryUsageHistory.size() >= MEMORY_HISTORY_SIZE) {
			memoryUsageHistory.erase(memoryUsageHistory.begin());
		}
		memoryUsageHistory.push_back(static_cast<GLfloat>(memoryUsage));

		// Display the memory usage graph
		ImGui::PlotLines("", memoryUsageHistory.data(), memoryUsageHistory.size(), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0, 100));

		// Display current memory usage value
		ImGui::Text("Current Memory Usage: %zu MB", memoryUsage);
	}

	if (ImGui::Button("Exit Game")) glfwSetWindowShouldClose(window, true);  // Close the game

	ImGui::End();

	glEnable(GL_DEPTH_TEST);
}

void main::cleanupImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void main::cleanup(shader& mainShader)
{
	mainShader.Delete();
}

void main::scroll_callback(GLFWwindow* window, GLdouble xoffset, GLdouble yoffset)
{
}

void main::mouse_callback(GLFWwindow* window, GLdouble xposIn, GLdouble yposIn)
{
	if (isGUIEnabled) {
		return;
	}

	GLfloat xpos = static_cast<GLfloat>(xposIn);
	GLfloat ypos = static_cast<GLfloat>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.updateCameraOrientation(camera.getYaw() + xoffset * 0.1f, camera.getPitch() + yoffset * 0.1f);
}

void main::mouseButtonCallback(GLFWwindow* window, GLint button, GLint action, GLint mods)
{
}

size_t main::getCurrentMemoryUsage()
{
	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	return pmc.PrivateUsage;
}

void main::processInput(GLFWwindow* window) 
{
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.setMovementState(Direction::FORWARD, true);
	else
		camera.setMovementState(Direction::FORWARD, false);

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.setMovementState(Direction::BACKWARD, true);
	else
		camera.setMovementState(Direction::BACKWARD, false);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.setMovementState(Direction::LEFT, true);
	else
		camera.setMovementState(Direction::LEFT, false);

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.setMovementState(Direction::RIGHT, true);
	else
		camera.setMovementState(Direction::RIGHT, false);

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.setMovementState(Direction::UP, true);
	else
		camera.setMovementState(Direction::UP, false);

	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		camera.setMovementState(Direction::DOWN, true);
	else
		camera.setMovementState(Direction::DOWN, false);
}

