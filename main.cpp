#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Model.h"
#include "stb/stb_image.h"

//the codes are learned from the website "www.learnopengl.com", credits to the website(author: Joey de Vries, address: "www.learnopengl.com").

const unsigned int WIDTH = 1024;
const unsigned int HEIGHT = 1024;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = (float)WIDTH / 2.0;
float lastY = (float)HEIGHT / 2.0;
bool firstMouse = true;

float yaw = -90.0f;
float pitch = 0.0f;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void processInput(GLFWwindow* window);

//camera
glm::vec3 cameraPos = glm::vec3(0.0, 0.0, 5.0);
glm::vec3 cameraFront = glm::vec3(0.0, 0.0, -1.0);
glm::vec3 cameraUp = glm::vec3(0.0, 1.0, 0.0);
glm::vec3 cameraRight = glm::vec3(0.0, 0.0, 0.0);

//light
glm::vec3 lightPos = glm::vec3(-2.0, 4.0, 2.0);

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Screen", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to open window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Failed to initilzie GLAD" << endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	Shader defaultShader("default.vert", "default.frag");
	Shader shadowShader("shadow.vert", "shadow.frag");
	Shader skyShader("sky.vert", "sky.frag");
	Model background("resource/combineBirdRock.obj");
	Model skybox("resource/skybox.obj");

	const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	defaultShader.use();
	defaultShader.setInt("shadowMap",1);


	while (!glfwWindowShouldClose(window)) {

		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
	
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 1.0f, far_plane = 7.5f;
		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;

		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(0.4, 0.4, 0.4));
		model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0, 1.0, 0.0));

		shadowShader.use();
		shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		shadowShader.setMat4("model", model);
		background.Draw(shadowShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, WIDTH, HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/ (float)HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos+ cameraFront, cameraUp);
		defaultShader.use();
		defaultShader.setMat4("projection", projection);
		defaultShader.setMat4("view", view);
		defaultShader.setMat4("model", model);
		defaultShader.setVec3("lightPos", lightPos);
		defaultShader.setVec3("viewPos", cameraPos);
		defaultShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		background.Draw(defaultShader);

		glDepthFunc(GL_LEQUAL);
		skyShader.use();
		skyShader.setMat4("projection", projection);
		skyShader.setMat4("view", view);
		model = glm::translate(model, glm::vec3(0.0, 4.0f, 0.0));
		model = glm::scale(model, glm::vec3(5.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, (float)glfwGetTime() / 100, glm::vec3(0.0f, 1.0f, 0.0f));
		skyShader.setMat4("model", model);
		skybox.Draw(skyShader);
		glDepthFunc(GL_LESS);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
};


void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
		cameraPos += glm::vec3(0.0f, 0.0f, -1.0f) * 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= glm::vec3(0.0f, 0.0f, -1.0f) * 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))) * 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))) * 2.5f * deltaTime;

	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		lightPos += glm::vec3(0.0f, 0.0f, -1.0f) * 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		lightPos -= glm::vec3(0.0f, 0.0f, -1.0f) * 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		lightPos -= glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))) * 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		lightPos += glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))) * 2.5f * deltaTime;
};

void mouse_callback(GLFWwindow* window, double xPos, double yPos) {

	float xpos = static_cast<float>(xPos);
	float ypos = static_cast<float>(yPos);
	
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
	cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0, 1.0, 0.0)));
	cameraUp = glm::normalize(glm::cross(cameraRight, cameraFront));

}
