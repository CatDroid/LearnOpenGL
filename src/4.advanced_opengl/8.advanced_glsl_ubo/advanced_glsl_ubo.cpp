#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

    // build and compile shaders
    // -------------------------
    Shader shaderRed("8.advanced_glsl.vs", "8.red.fs");
    Shader shaderGreen("8.advanced_glsl.vs", "8.green.fs");
    Shader shaderBlue("8.advanced_glsl.vs", "8.blue.fs");
    Shader shaderYellow("8.advanced_glsl.vs", "8.yellow.fs");
    
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float cubeVertices[] = {
        // positions         
        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f,  
         0.5f,  0.5f, -0.5f,  
         0.5f,  0.5f, -0.5f,  
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
        -0.5f,  0.5f,  0.5f, 
        -0.5f, -0.5f,  0.5f, 

        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f, -0.5f, 
        -0.5f, -0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f, 

         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f, -0.5f,  
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  

        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f,  
         0.5f, -0.5f,  0.5f,  
         0.5f, -0.5f,  0.5f,  
        -0.5f, -0.5f,  0.5f, 
        -0.5f, -0.5f, -0.5f, 

        -0.5f,  0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f,  
         0.5f,  0.5f,  0.5f,  
         0.5f,  0.5f,  0.5f,  
        -0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f, -0.5f, 
    };
    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	/*
		UBO对象作用:
			1. 任何大型的渲染程序都可能同时激活有上百个着色器程序，
			    这时候Uniform缓冲对象的优势就会很大地体现出来了

			2. 比起在多个着色器中修改同样的uniform，在Uniform缓冲中修改一次会更容易一些

			3. 可以在着色器中使用更多的uniform。
			    OpenGL限制了它能够处理的uniform数量，
				这可以通过GL_MAX_VERTEX_UNIFORM_COMPONENTS来查询
	*/

    // configure a uniform buffer object
    // ---------------------------------
    // first. We get the relevant block indices 
	// 每个program中的"Matrices"一致块的句柄
	// Uniform块索引(Uniform Block Index)是着色器中已定义Uniform块的位置值索引。
	// 这可以通过调用glGetUniformBlockIndex来获取
    unsigned int uniformBlockIndexRed = glGetUniformBlockIndex(shaderRed.ID, "Matrices");
    unsigned int uniformBlockIndexGreen = glGetUniformBlockIndex(shaderGreen.ID, "Matrices");
    unsigned int uniformBlockIndexBlue = glGetUniformBlockIndex(shaderBlue.ID, "Matrices");
    unsigned int uniformBlockIndexYellow = glGetUniformBlockIndex(shaderYellow.ID, "Matrices");

	// 在OpenGL上下文中，定义了一些绑定点(Binding Point)
	// 在使用上, 一个绑定点 对应一个 UBO(链接到一个UBO), 相当于一个绑定点代表某种"功能"(比如绑定点0对应的是VP矩阵)
	// 然后不同的program的对应uniform句柄(值不一定一样), 然后绑定到绑定点
#define GL_CONTEXT_BIND_POINT_0 0 

	// OpengGL4.2 shader中可以直接指定某个一致块的绑定点 不用调用 glGetUniformBlockIndex 和 glUniformBlockBinding
	// 添加一个布局标识符  binding = 2
	// layout(std140, binding = 2) uniform Lights { ... };

	// 每个program中的一致块句柄 绑定到其中一个绑定点
	// glUniformBlockBinding(着色器的句柄, 着色器中该一致块的句柄, 绑定点)
    // then we link each shader's uniform block to this uniform binding point
    glUniformBlockBinding(shaderRed.ID,		uniformBlockIndexRed,			GL_CONTEXT_BIND_POINT_0);
    glUniformBlockBinding(shaderGreen.ID,	uniformBlockIndexGreen,		GL_CONTEXT_BIND_POINT_0);
    glUniformBlockBinding(shaderBlue.ID,		uniformBlockIndexBlue,			GL_CONTEXT_BIND_POINT_0);
    glUniformBlockBinding(shaderYellow.ID,	uniformBlockIndexYellow,		GL_CONTEXT_BIND_POINT_0);

    // Now actually create the buffer 创建UBO, 大小刚好是两个Mat4的size
    unsigned int uboMatrices;
    glGenBuffers(1, &uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


	// 把UBO对象链接到一个绑定点 
    // define the range of the buffer that links to a uniform binding point --可以一个UBO的不同部分对应不同的绑定点
    glBindBufferRange(GL_UNIFORM_BUFFER, GL_CONTEXT_BIND_POINT_0, uboMatrices,  0,  2 * sizeof(glm::mat4));
	// glBindBufferBase(GL_UNIFORM_BUFFER, GL_CONTEXT_BIND_POINT_0, uboExampleBlock); 

	// 投影矩阵只更新一次 (导致了 投影矩阵的视野(Field of View)值保持不变, 所以摄像机就没有缩放了)
    // store the projection matrix (we only do this once now) (note: we're not using zoom anymore by changing the FoV)
    glm::mat4 projection = glm::perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  
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

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 每次只更新ubo对象中的view矩阵 (偏移+大小)
        // set the view and projection matrix in the uniform block - we only have to do this once per loop iteration.
        glm::mat4 view = camera.GetViewMatrix();
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // draw 4 cubes 
        // RED
        glBindVertexArray(cubeVAO);
        shaderRed.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.75f, 0.75f, 0.0f)); // move top-left
        shaderRed.setMat4("model", model); // 不同物体的模型矩阵还是要单独传入
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // GREEN
        shaderGreen.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.75f, 0.75f, 0.0f)); // move top-right
        shaderGreen.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // YELLOW
        shaderYellow.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.75f, -0.75f, 0.0f)); // move bottom-left
        shaderYellow.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        // BLUE
        shaderBlue.use();
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.75f, -0.75f, 0.0f)); // move bottom-right
        shaderBlue.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);

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
