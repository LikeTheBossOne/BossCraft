#include <iostream>
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "Camera.h"
#include "World.h"
#include "load_stb_image.h"
#include <chrono>
#include "GlobalEventManager.h"
#include "JobSystem.h"

unsigned int SCREEN_WIDTH = 800;
unsigned int SCREEN_HEIGHT = 600;
World* world = nullptr;

bool firstMouse = true;
float lastX = 0;
float lastY = 0;

bool wireframeMode = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void MouseMovedCallback(GLFWwindow* window, double xPos, double yPos)
{
	if (firstMouse) // initially set to true
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	float xOffset = xPos - lastX;
	float yOffset = lastY - yPos; // Y ranges from bottom to top
	lastX = xPos;
	lastY = yPos;

	world->GetCamera()->ProcessMouseMovement(xOffset, yOffset, true);
}

void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	world->GetCamera()->ProcessMouseScroll(yOffset);
}

GLFWwindow* CreateWindow()
{
	// Setup window
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "BossCraft", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return nullptr;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetCursorPosCallback(window, MouseMovedCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	return window;
}

void LoadTexture(const char* pathToTexture, unsigned int* textureID, int rgba, int glRepeat)
{
	glGenTextures(1, textureID);
	glBindTexture(GL_TEXTURE_2D, *textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glRepeat);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glRepeat);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(pathToTexture, &width, &height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, rgba, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
}

void RenderLoop(GLFWwindow* window)
{
	float dt = 0.0f;
	int fpsCounts = 0;
	while (!glfwWindowShouldClose(window))
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, 1);
		}
		if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS/* && (wireframeCooldown > 0.1f)*/)
		{
			//wireframeCooldown = 0;
			wireframeMode = !wireframeMode;
			if (wireframeMode)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			world->GetCamera()->ProcessKeyBoard(CameraDirection::Forward, .1);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			world->GetCamera()->ProcessKeyBoard(CameraDirection::Backward, .1);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			world->GetCamera()->ProcessKeyBoard(CameraDirection::Left, .1);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			world->GetCamera()->ProcessKeyBoard(CameraDirection::Right, .1);
		}

		GlobalEventManager::ProcessEvents();
		
		world->Update(0.f);

		glfwSwapBuffers(window);
		glfwPollEvents();


		auto stopTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();

		fpsCounts++;
		//if (fpsCounts > 6)
		//{
			//std::cout << dt * 1000 << std::endl;
			
			fpsCounts = 0;
		//}
		
	}
}

int main()
{
    std::cout << "Hello World!\n";

	GlobalEventManager::Init();
	JobSystem::Init();
	
	GLFWwindow* window = CreateWindow();

	unsigned int textureID;
	LoadTexture("Resources/textures/block/dirt.png", &textureID, GL_RGBA, GL_CLAMP_TO_EDGE);
	
	world = new World(new Shader("Shaders\\vertex1.vs", "Shaders\\fragment1.fs"), textureID);

	RenderLoop(window);
}