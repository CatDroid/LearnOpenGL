#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path, bool gammaCorrection);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

 

bool framebufferGammaEnabled = false;
bool framebufferGammaEnabledPressed = false;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void logHint()
{
	{
	 
		static bool sframebufferGammaEnabled = framebufferGammaEnabled;
		bool log = false;

		if ( sframebufferGammaEnabled != framebufferGammaEnabled)
		{
			 
			sframebufferGammaEnabled = framebufferGammaEnabled;

			log = true;
		}
		else
		{
			static uint32_t lessPrint = 0;
			if ((lessPrint++ & (uint32_t)0xFFFF) == 0)
			{
				log = true;
			}
		}

		if (log)
		{
			std::cout << "hints ----------------------------------------- " << std::endl;
 
			std::cout << (framebufferGammaEnabled ? "Fbo Gamma enabled" : "Fbo Gamma disabled")
				<< ", Press  B switch"
				<< std::endl;
		}
	}
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	/*
	OpenGL: If enabled and supported by the system, 
				the GL_FRAMEBUFFER_SRGB enable will control sRGB rendering.
				By default, sRGB rendering will be disabled.
	
	OpenGL ES: If enabled and supported by the system, 
				the context will always have sRGB rendering enabled.
				
				??????OpenglES ?????????sRGB???
				??????OpenGL sRGB??????????????????

	 ???????????????PC??? ??????glEnable(GL_FRAMEBUFFER_SRGB)???????????????

	?????????:
	Applications may wish to use sRGB format default framebuffers to
	more easily achieve sRGB rendering to display devices. This
	extension allows creating EGLSurfaces which will be rendered to in
	sRGB by OpenGL contexts supporting that capability.

	????????????EGLSurface??????sRGB??????
	EGL??????  KHR_gl_colorspace/EGL_KHR_gl_colorspace  EGL 1.4 is required.
	https://registry.khronos.org/EGL/extensions/KHR/EGL_KHR_gl_colorspace.txt
	eglCreateWindowSurface,
	eglCreatePbufferSurface
	eglCreatePixmapSurface
	????????????
		EGL_GL_COLORSPACE_KHR						0x309D
	????????????
		EGL_GL_COLORSPACE_SRGB_KHR              0x3089
		EGL_GL_COLORSPACE_LINEAR_KHR			0x308A

	EGL_GL_COLORSPACE_KHR ???????????????EGL_GL_COLORSPACE_LINEAR_KHR

	EGL_GL_COLORSPACE_KHR ?????????????????? sRGB ??????????????? OpenGL ??? OpenGL ES ?????????????????? 
	EGL ???????????????????????????????????????(colorspace models)???
	EGL_GL_COLORSPACE_KHR ????????? OpenGL ??? OpenGL ES ?????????????????????(rendering to the surface)?????????????????????
	
	?????????????????? EGL_GL_COLORSPACE_SRGB_KHR
	??????????????????????????????(non-linear)??????????????????(perceptually uniform)??????????????????
	????????? GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING ?????? GL_SRGB

	?????????????????? EGL_GL_COLORSPACE_LINEAR_KHR
	????????????????????????????????????
	????????? GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING ??????GL_LINEAR

	?????? OpenGL ???????????????, ?????????????????? GL_FRAMEBUFFER_SRGB ????????? sRGB ?????????
	??????????????? sRGB ?????????
	 OpenGL ES?????????????????????????????????

	*/
	//glfwWindowHint(GLFW_SRGB_CAPABLE, true);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    // build and compile shaders
    // -------------------------
    Shader shader("2.1.gamma_correction.vs", "2.1.gamma_correction.fs");
	Shader screenShader("2.1.screen.vs", "2.1.screen.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float planeVertices[] = {
        // positions            // normals         // texcoords
         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,

         10.0f, -0.5f,  10.0f,  0.0f, 1.0f, 0.0f,  10.0f,  0.0f,
        -10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,   0.0f, 10.0f,
         10.0f, -0.5f, -10.0f,  0.0f, 1.0f, 0.0f,  10.0f, 10.0f
    };
    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

	float quadVertices[] = { 
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};
	unsigned int quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


    // load textures
    // -------------
    unsigned int floorTextureGammaCorrected 
		= loadTexture(FileSystem::getPath("resources/textures/wood.png").c_str(), true);

	/*
	
	GL_FRAMEBUFFER_SRGB????????????OpenGL????????????

	?????????????????????????????????sRGB??????????????????????????????????????????????????????(?????????)?????????sRGB?????????
	????????????"?????????sRGB??????"?????????????????????GL_FRAMEBUFFER_SRGB?????????sRGB?????????

	???OpenGL ES ??????framebuffer??????sRGB??????
	?????????????????????????????????, ??????????????????????????????GL_FRAMEBUFFER_SRGB???OpenGL
	OpenGL ES ????????? "EXT_sRGB_write_control " ???????????????????

	!!glfw?????????backbuffer Color ?????????SRGB????????????GL_FRAMEBUFFER_SRGB??????????????????


	*/

	// SRGB?????????fbo
	GLuint sRgbTexture;
	glGenTextures(1, &sRgbTexture);
	glBindTexture(GL_TEXTURE_2D, sRgbTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// ???????????????SRGB
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	GLuint sRgbFBO;
	glGenFramebuffers(1, &sRgbFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, sRgbFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sRgbTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	 
	// ???SRGB?????????fbo
	GLuint rgbTexture;
	glGenTextures(1, &rgbTexture);
	glBindTexture(GL_TEXTURE_2D, rgbTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned int rbo1;
	glGenRenderbuffers(1, &rbo1);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo1);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	GLuint rgbFBO;
	glGenFramebuffers(1, &rgbFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, rgbFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rgbTexture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo1);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;


	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // shader configuration
    // --------------------
    shader.use();
    shader.setInt("floorTexture", 0);

    // lighting info
    // -------------
    glm::vec3 lightPositions[] = {
        glm::vec3(-3.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3 (1.0f, 0.0f, 0.0f),
        glm::vec3 (3.0f, 0.0f, 0.0f)
    };
    glm::vec3 lightColors[] = {
        glm::vec3(0.25),
        glm::vec3(0.50),
        glm::vec3(0.75),
        glm::vec3(1.00)
    };

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

		/*
		if (framebufferGammaEnabled)
		{
			glEnable(GL_FRAMEBUFFER_SRGB);
		}
		else
		{
			glDisable(GL_FRAMEBUFFER_SRGB);
		}
		*/

		glBindFramebuffer(GL_FRAMEBUFFER, sRgbFBO);
		//glBindFramebuffer(GL_FRAMEBUFFER, rgbFBO);
		

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);

		// PC???????????????sRGB ?????? fbo??????????????????SRGB+??????glEnable(GL_FRAMEBUFFER_SRGB)
		glEnable(GL_FRAMEBUFFER_SRGB);


        // draw objects
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        // set light uniforms
        glUniform3fv(glGetUniformLocation(shader.ID, "lightPositions"), 4, &lightPositions[0][0]);
        glUniform3fv(glGetUniformLocation(shader.ID, "lightColors"), 4, &lightColors[0][0]);
        shader.setVec3("viewPos", camera.Position);
      
        // floor
        glBindVertexArray(planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTextureGammaCorrected );
        glDrawArrays(GL_TRIANGLES, 0, 6);


		// screen 
		glDisable(GL_FRAMEBUFFER_SRGB);
		//glEnable(GL_FRAMEBUFFER_SRGB); // ?????????backbuffer???srgb????????????

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(quadVAO);

		screenShader.use();
		screenShader.setInt("screenTexture", 0);
		glActiveTexture(GL_TEXTURE0);
		// ?????? ???????????????SRGB??? ??????????????????????????????,???????????????????????????
		// ??????
	    // 1. ?????????????????????????????????,??????fbo????????????SRGB
		// 2. ????????????????????????????????? ????????????draw
		glBindTexture(GL_TEXTURE_2D, sRgbTexture);
		//glBindTexture(GL_TEXTURE_2D, rgbTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		logHint();
	
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

	{
		// ??????fbo sRBG?????????shader????????????
		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !framebufferGammaEnabledPressed)
		{
			framebufferGammaEnabledPressed = true;

			framebufferGammaEnabled = !framebufferGammaEnabled;
		 
		}

		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
		{
			framebufferGammaEnabledPressed = false;
		} 
	}

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
			
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
			/* 
				??????
				    1. ????????????????????????, ?????????????????????????????????, ?????????????????????, ???????????????SRGB
					1. ???????????????????????????????????? GL_SRGB !! ??????????????????GL_RGB
					2. apha???????????????????????? ????????????alpha??????????????? ?????? GL_SRGB_ALPHA
					3. ???????????????????????????sRGB??????
						diffuse??????????????????????????????????????????????????????sRGB????????????
						specular????????????????????????????????????????????????	
			*/
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
