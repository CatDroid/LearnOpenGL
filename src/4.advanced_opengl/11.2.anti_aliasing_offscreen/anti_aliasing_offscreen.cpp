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


void blitToScreen(GLuint fromFbo, int width, int height)
{
    /*
     
     void glDrawBuffer(GLenum buf); // 设置到上下文 ???
     void glNamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf); // 设置到给定的fbo??
     
     对于默认缓冲区 可以指定4个颜色缓冲来写入: (前后左右,GL_NONE)
        GL_NONE, GL_FRONT_LEFT, GL_FRONT_RIGHT, GL_BACK_LEFT, GL_BACK_RIGHT, GL_FRONT, GL_BACK, GL_LEFT, GL_RIGHT, and GL_FRONT_AND_BACK
     
        对于单缓冲，初始值是 GL_FRONT
        对于双缓冲，初始值是 GL_BACK
     
     对于FBO对象, 可选值是 GL_COLOR_ATTACHMENT[n] 或者 GL_NONE
     
     */
    
    
    // 绘制到屏幕(fbo=0)的back缓冲区
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    
    // 从 fromFBO的 color0附件
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fromFbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
   
    // 只拷贝颜色附件
    glBlitFramebuffer(0, 0, width, height,
                      0, 0, width, height,
                      GL_COLOR_BUFFER_BIT,// 颜色附件
                      GL_NEAREST
                      );
    
    // 读写帧缓冲都设置为0
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#define CHECK_ERROR  {auto error = glGetError(); if (error != GL_NO_ERROR) { printf("gl error = 0x%x (line = %d)\n", error, __LINE__); }};


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

	// 光栅器会将一个图元的所有顶点作为输入，并将它转换为一系列的片元
	// 光栅器必须以某种方式来决定 "每个顶点" 最终所在的"片元/屏幕坐标"
	
	// 一个屏幕像素的网格，每个像素的中心包含有一个采样点(Sample Point), 它会被用来决定''这个三角形''是否''遮盖了某个像素''。
	// 多重采样所做的正是将"单一的采样点"变为"多个采样点"
	// 我们不再使用"像素中心的单一采样点"，取而代之的是以特定图案排列的4个"子采样点(Subsample)"
	// 这也意味着颜色缓冲的大小会随着子采样点的增加而增加
	//
	// MSAA真正的工作方式是，无论三角形遮盖了多少个子采样点，（每个图元中）每个像素只运行一次片段着色器。
	// 片段着色器所使用的顶点数据会插值到""每个像素的中心""，所得到的结果颜色会被储存在""每个被遮盖住的子采样点""中
	// (!!也就是以像素中心作为插值位置!!)                                    (!!只会画一次,保存到覆盖的子采样点中!!)


	// 对深度测试来说，每个顶点的深度值会在"运行深度测试"之前被"插值"到"各个子样本"中 
	// 对模板测试来说，我们对每个子样本，而不是每个像素，存储一个模板值。
	// 当然，这也意味着""深度和模板缓冲的大小""会乘以""子采样点的个数""。

	// 多重采样的缓冲(纹理, 渲染缓冲)，将其作为帧缓冲的附件

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader shader("11.2.anti_aliasing.vs", "11.2.anti_aliasing.fs");
    Shader screenShader("11.2.aa_post.vs", "11.2.aa_post.fs");

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
        -0.5f,  0.5f, -0.5f
    };
    float quadVertices[] = {   // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    // setup cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // setup screen VAO
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


    // configure MSAA framebuffer
    // --------------------------
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

#define MULTI_SAMPLE_NUMBER 4 

    // create a multisampled color attachment texture
    unsigned int textureColorBufferMultiSampled;
    glGenTextures(1, &textureColorBufferMultiSampled);
	// !! 注意这个纹理不只是Texture2D还是MultiSample目标类型
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    // !! 注意分配内存也不是 glTexImage2D 而是专门的  glTexImage2D+Multisample 参数增加了子样本数目=4
	//    对于纹理 还增加了一个参数 fixedsamplelocations,  rbo就没有(不能用于采样)
	// 
	//    fixedsamplelocations = GL_TRUE，图像将会对每个纹素使用相同的样本位置以及相同数量的子采样点个数 ???
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MULTI_SAMPLE_NUMBER , GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);


	// !! 绑定还是 glFramebufferTexture2D 但是 纹理目标是 GL_TEXTURE_2D_MULTISAMPLE
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
    CHECK_ERROR

    // 作用在当前绑定的fbo 
    // 无论使用的FBO是何种类型，在数组中除GL_NONE以外的值最多只能使用一次
    // RenderDoc 会看到 slot 0 是 Fbo附件1对应的颜色纹理， slot是从shader看过去
    GLenum swizzle[] = {GL_COLOR_ATTACHMENT1,  GL_NONE};
    glDrawBuffers(sizeof(swizzle)/sizeof(swizzle[0]), swizzle);
    CHECK_ERROR

	// !! rbo也可以是多重采样的  格式还是D24S8  
	// create a (also multisampled) renderbuffer object for depth and stencil attachments
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	// !! 注意分配内存 也不是 glRenderbufferStorage 而是专门的 glRenderbufferStorage+Multisample  参数增加了子样本数目=4
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, MULTI_SAMPLE_NUMBER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// 绑定rbo使用 glFramebuffer+Renderbuffer 不是 glFramebuffer+Texture2D
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // configure second post-processing framebuffer
    unsigned int intermediateFBO;
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    // create a color attachment texture
    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);	// we only need a color buffer

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // shader configuration
    // --------------------
    shader.use();
    screenShader.setInt("screenTexture", 0);

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

        // 1. draw scene as normal in multisampled buffers
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);


        // set transformation matrices		
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        shader.setMat4("projection", projection);
        shader.setMat4("view", camera.GetViewMatrix());
        shader.setMat4("model", glm::mat4(1.0f));

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

		// !! 因为多重采样缓冲有一点特别，我们不能直接将它们的缓冲图像用于其他运算，比如在着色器中对它们进行采样
		//    一个多重采样的图像包含比普通图像更多的信息，我们所要做的是缩小或者还原(Resolve)图像。
		//   多重采样帧缓冲的还原通常是通过glBlitFramebuffer来完成

		//  另外， 将一个多重采样的纹理图像不进行还原直接传入着色器也是可行的
		//  GLSL提供了这样的选项，让我们能够对纹理图像的每个子样本进行采样，所以我们可以创建我们自己的抗锯齿算法
		//  要想获取每个子样本的颜色值，你需要将纹理uniform采样器设置为sampler2DMS，而不是平常使用的sampler2D
		// 
		//  uniform sampler2DMS screenTextureMS;
		//  vec4 colorSample = texelFetch(screenTextureMS, TexCoords, 3);  // 第4个子样本

        // 2. now blit multisampled buffer(s) to normal colorbuffer of intermediate FBO. Image is stored in screenTexture
        //    读取和绘制的帧缓冲目标 分开绑定
        
        
        //glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        //glReadBuffer(GL_COLOR_ATTACHMENT1);

        // 设置读写的fbo
		glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        // 设置读写的附件  修改的是当前绑定GL_READ_FRAMEBUFFER的fbo (RenderDoc抓帧可以看到), 并不是context的变量 
        // glReadBuffer 影响这个fbo, 在执行glReadPixelBuffer  glBlitFrameBuffer   glCopyTexImage2D 等命令是对哪个附件操作
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

		//     将多重采样缓冲位块(Blit)传送到一个没有使用多重采样纹理附件的FBO
		//     然而，这也意味着我们需要生成一个新的FBO，作为中介帧缓冲对象
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        
        /*
            glBlitFrameBuffer每次只拷贝一个附件(缓冲区),需要使用glReadBuffer和glDrawBuffer来指定
         
            还可以指定 深度和模板附件/缓冲区
            #define GL_DEPTH_BUFFER_BIT 0x00000100
            #define GL_STENCIL_BUFFER_BIT 0x00000400
         
            for (int i = 0; i < Attachments.size(); ++i)
            {
                glReadBuffer(Attachments[i]);
                glDrawBuffer(Attachments[i]);

                glBlitFramebuffer(0, 0,
                                        width,
                                        height,
                                        0, 0,
                                        width,
                                        height,
                                        GL_COLOR_BUFFER_BIT,
                                        GL_NEAREST);
            }
         
         */
        
        // 3. now render quad with scene's visuals as its texture image
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // 同时绑定了读取和绘制的帧缓冲目标
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

		// !! 像是边缘检测这样的后期处理滤镜会重新导致锯齿
        // draw Screen quad
        screenShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture); // use the now resolved color attachment as the quad's texture
        glDrawArrays(GL_TRIANGLES, 0, 6);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
