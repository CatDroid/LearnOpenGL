#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// global flag 
bool g_PolygonModeLineOrFill = false; //  false: 图元填充方式  ture: 线框填充方式 


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

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
    //Shader shader("9.1.geometry_shader.vs", "9.1.geometry_shader.fs"); // , "9.1.geometry_shader.gs"
	Shader shader("9.1.geometry_shader.vs", "9.1.geometry_shader.fs", "9.1.geometry_shader.gs");  
    //Shader triShader("9.1.triangle_shader.vs", "9.1.triangle_shader.fs");
    Shader triShader("9.1.triangle_shader.vs", "9.1.triangle_shader.fs", "9.1.triangle_shader.gs");
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float points[] = {
        -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, // top-left
         0.5f,  0.5f, 0.0f, 1.0f, 0.0f, // top-right
         0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // bottom-right
        -0.5f, -0.5f, 1.0f, 1.0f, 0.0f  // bottom-left
    };
    unsigned int VBO, VAO;
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), &points, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);


 
    float pointsTri[] = {
        -0.25f,  0.25f, 1.0f, 0.0f, 0.0f, // top-left
        0.25f,  0.25f, 0.0f, 1.0f, 0.0f, // top-right
        0.25f, -0.25f, 0.0f, 0.0f, 1.0f, // bottom-right
    };
    unsigned int VBOtri, VAOtri;
    glGenBuffers(1, &VBOtri);
    glGenVertexArrays(1, &VAOtri);
    glBindBuffer(GL_ARRAY_BUFFER, VBOtri);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointsTri), &pointsTri, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(VAOtri);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBOtri);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

	// 在顶点着色器中修改点大小的功能默认是禁用的，
	// 如果你需要启用它的话，你需要启用OpenGL的GL_PROGRAM_POINT_SIZE：
	//glEnable(GL_PROGRAM_POINT_SIZE);

	// 或者不在shader中执行, 而在cpu端修改管线状态
	// glPointSize(50);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {

        // input
        // -----
        processInput(window);


        if (g_PolygonModeLineOrFill)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }


        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw points
        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, 4); 

        // 修改为默认
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


        // https://stackoverflow.com/questions/3539205/is-there-a-substitute-for-glpolygonmode-in-open-gl-es-webgl
        // OpenGLES 没有 glPolygonMode
        // a. 修改顶点 
        //      对于索引渲染    glDrawElements...  调用中使用不同的索引数组，或者
        //      对于非索引渲染（glDrawArrays...），完全使用不同的顶点数组
        // b. 使用插入几何着色器的方式 
        //     WebGL 似乎不支持它们，OpenGL ES 应该从 3.2 开始支持，Desptop GL 也是如此但有旧的glPolygonMode 支持
        //     vs和ps之间安装一个相当简单的几何着色器，它采用三角形并为每个三角形输出 3 条线

        // 无论使用 glPolygonMode 还是 gs，线条绘制都会走 片段着色器 

        triShader.use();
        glBindVertexArray(VAOtri);
        glDrawArrays(GL_TRIANGLES, 0, 3); // // start=0 count=3个顶点


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}


 

void processInput(GLFWwindow* window)
{

#define _TRACE_STR(s) #s
#define TRACE_STR(s) _TRACE_STR(s)

#define MyKeyHandler(KEY, VARIABLE,  KEY_STR, INFO_STR) { \
		static bool sKeyPress = false; \
		bool KeyPressed = glfwGetKey(window, KEY);\
		if (KeyPressed == GLFW_PRESS && !sKeyPress)\
		{\
			VARIABLE = !VARIABLE;\
			sKeyPress = true;\
		}\
		else if (KeyPressed == GLFW_RELEASE)\
		{\
			sKeyPress = false;\
		} \
        \
        static decltype(VARIABLE) sStatus = !VARIABLE; \
        if (sStatus != VARIABLE)\
        {\
            sStatus = VARIABLE; \
            printf("Press Key "  KEY_STR ", " TRACE_STR(VARIABLE) " = %d, " INFO_STR  "\n", VARIABLE); \
        } \
        }

        // 图元填充方式  填充模式 还是 线框模式
        MyKeyHandler(GLFW_KEY_P,  g_PolygonModeLineOrFill, "P", "false: fill mode,  true: line mode");

    
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
