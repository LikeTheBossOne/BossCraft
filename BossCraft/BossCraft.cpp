#include <iostream>
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "Camera.h"
#include "World.h"
#include "load_stb_image.h"
#include <chrono>

#include "BlockProvider.h"
#include "ChunkResources.h"
#include "GlobalEventManager.h"
#include "JobSystem.h"
#include "FastNoiseLite.h"
#include "Player.h"
#include "TextureAtlas.h"

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
	glfwSwapInterval(0); // Disable VSync

	return window;
}

void LoadTextureAtlas(const char* pathToTexture, unsigned int* textureID, int rgba, int glRepeat, int* width, int* height)
{
	glGenTextures(1, textureID);
	glBindTexture(GL_TEXTURE_2D, *textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glRepeat);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glRepeat);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int nrChannels;
	unsigned char* data = stbi_load(pathToTexture, width, height, &nrChannels, 0);

	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, *width, *height, 0, rgba, GL_UNSIGNED_BYTE, data);
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
			world->GetPlayer()->ProcessKeyBoard(CameraDirection::Forward, 10, dt);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			world->GetPlayer()->ProcessKeyBoard(CameraDirection::Backward, 10, dt);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			world->GetPlayer()->ProcessKeyBoard(CameraDirection::Left, 10, dt);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			world->GetPlayer()->ProcessKeyBoard(CameraDirection::Right, 10, dt);
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
		{
			world->GetPlayer()->ProcessLeftMouseClick();
		}
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT))
		{
			world->GetPlayer()->ProcessRightMouseClick();
		}

		GlobalEventManager::ProcessEvents();
		
		world->Update(dt);

		glfwSwapBuffers(window);
		glfwPollEvents();


		auto stopTime = std::chrono::high_resolution_clock::now();

		dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();

		fpsCounts++;
		if (fpsCounts > 6)
		{
			std::cout << dt * 1000 << std::endl;
			
			fpsCounts = 0;
		}
		
	}
}

int main()
{
    std::cout << "Hello World!\n";
	
	GlobalEventManager::Init();
	JobSystem::Init();
	BlockProvider::Init();
	ChunkResources::Init("chunks");
	
	GLFWwindow* window = CreateWindow();

	unsigned int textureID;
	int width, height;
	//LoadTextureAtlas("Resources/atlas.png", &textureID, GL_RGBA, GL_CLAMP_TO_EDGE, &width, &height);
	TextureAtlas* atlas = new TextureAtlas("Resources/atlas.png", 16, 16);
	
	world = new World(new Shader("Shaders\\vertex2.vs", "Shaders\\fragment2.fs"), atlas, new Player(glm::vec3(0, 64, 0)));

	RenderLoop(window);
}