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
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);

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
    //
    // 一个模板缓冲中，（通常）每个模板值(Stencil Value)是8位的。
    // 所以每个像素/片段一共能有256种不同的模板值。
    // 我们可以将这些模板值设置为我们想要的值，然后当某一个片段有某一个模板值的时候，我们就可以选择丢弃或是保留这个片段了
    // 通过使用模板缓冲，我们可以根据场景中已绘制的其它物体的片段，来决定是否丢弃特定的片段
    //
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

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);  // 打开深度测试
    glDepthFunc(GL_LESS);
    
    glEnable(GL_STENCIL_TEST); // 打开模板测试  模板测试是先做深度测试
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF); // 参考值是1  比较前当前值 和 缓冲的值都先用0xFF作AND操作 再比较 不等于就通过
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // 如果模板测试和深度测试都通过了，那么我们希望将储存的模板值设置为参考值
    
    /*
        glStencilMask允许我们设置一个位掩码(Bitmask)，它会与将要写入缓冲的模板值进行与(AND)运算()。
        默认情况下设置的位掩码所有位都为1，不影响输出，
        但如果我们将它设置为0x00，写入缓冲的所有模板值最后都会变成0.  这与深度测试中的glDepthMask(GL_FALSE)是等价的
     
        glStencilMask(GLuint mask);   参数是个无符号整型 但也是个开关，是每个bit的开光
        glDepthMask(GLboolean flag);  参数只是个true/false开关
     
     
     
        glEnable(GL_STENCIL_TEST);                      --- 开关模板测试
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);            --- 怎么比较
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);      --- 比较结果怎么处理
        glStencilMask(GLuint mask);                     --- 是否写入
     
     
     
        如果两个箱子很近，这样边框会重合(不能每个盒子都有完整边框)
        如果你想让每个物体都有一个完整的边框，你需要对每个物体都清空模板缓冲，并有创意地利用深度缓冲。
        物体轮廓算法在需要显示选中物体的游戏（想想策略游戏）中非常常见
        也可以使用后期处理滤镜(Filter)，像是高斯模糊(Gaussian Blur)，让边框看起来更自然。
     
     
        用途
            物体轮廓
            后视镜中绘制纹理，让它能够绘制到镜子形状中
            阴影体积(Shadow Volume)的模板缓冲技术渲染实时阴影
     
     */
    



    // build and compile shaders
    // -------------------------
    Shader shader("2.stencil_testing.vs", "2.stencil_testing.fs");
    Shader shaderSingleColor("2.stencil_testing.vs", "2.stencil_single_color.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float cubeVertices[] = {
        // positions          // texture Coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    float planeVertices[] = {
        // positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

         5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
        -5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
         5.0f, -0.5f, -5.0f,  2.0f, 2.0f
    };
    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    // plane VAO
    unsigned int planeVAO, planeVBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    // load textures
    // -------------
    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/marble.jpg").c_str());
    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/metal.png").c_str());

    // shader configuration
    // --------------------
    shader.use();
    shader.setInt("texture1", 0);

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
        //glClearStencil(0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // don't forget to clear the stencil buffer!

        // set uniforms
        shaderSingleColor.use();
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        shaderSingleColor.setMat4("view", view);
        shaderSingleColor.setMat4("projection", projection);

        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // draw floor as normal, but don't write the floor to the stencil buffer, we only care about the containers. We set its mask to 0x00 to not write to the stencil buffer.
        glStencilMask(0x00);
        // floor
        glBindVertexArray(planeVAO);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        shader.setMat4("model", glm::mat4(1.0f));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // 1st. render pass, draw objects as normal, writing to the stencil buffer
        // --------------------------------------------------------------------
        //glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        // cubes
        glStencilFunc(GL_ALWAYS, 1, 0xFF); //  xxx "修改成每个物体单独的模板参考值"这个不行，因为每个物体都用NOT_EQUAL来对比
        
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        //glStencilFunc(GL_ALWAYS, 2, 0xFF); // 修改成每个物体单独的模板参考值
        model = glm::mat4(1.0f);
        model = glm::rotate(model, 3.14f / 4.0f , glm::vec3(0.0,1.0,0.0));
        model = glm::translate(model, glm::vec3(-0.1f, 0.0f, 0.0f)); // 如果两个箱子很近，这样边框会重合(不能每个盒子都有完整边框)
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 2nd. render pass: now draw slightly scaled versions of the objects, this time disabling stencil writing.
        // Because the stencil buffer is now filled with several 1s. The parts of the buffer that are 1 are not drawn, thus only drawing 
        // the objects' size differences, making it look like borders.
        // -----------------------------------------------------------------------------------------------------------------------------
        /*
         
            模板缓冲应该通过还是失败(glStencilFunc)，以及它应该如何影响模板缓冲(glStencilOp)
         
            glStencilFunc(GLenum func, GLint ref, GLuint mask)
            glStencilFunc(GL_EQUAL, 1, 0xFF)
            func：设置模板测试函数(Stencil Test Function)
            ref：设置了模板测试的参考值(Reference Value)。模板缓冲的内容将会与这个值进行比较
            mask：设置一个掩码，它将会与参考值和储存的模板值在测试比较它们之前进行与(AND)运算。初始情况下所有位都为1
         
            模板缓冲的内容将会与这个值进行比较
         */
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        /*
            glStencilMask(0x00)不仅会阻止模板缓冲的写入，也会阻止其清空(glClear(stencil_buffer)无效)
            跟glDepthMask(GL_FALSE)类似 都会影响glClear
         */
        glStencilMask(0x00);
        
        /*
            glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass)  --- 只是影响模板缓冲
            sfail：模板测试失败时采取的行为。
            dpfail：模板测试通过，但深度测试失败时采取的行为
            dppass：模板测试和深度测试都通过时采取的行为
         
            GL_KEEP         保持当前储存的模板值
            GL_ZERO         将模板值设置为0
            GL_REPLACE      将模板值设置为glStencilFunc函数设置的ref值
            GL_INVERT       按位翻转当前的模板缓冲值
            GL_INCR         如果模板值小于最大值则将模板值加1
            GL_INCR_WRAP    与GL_INCR一样，但如果模板值超过了最大值则归零
         
            默认情况下glStencilOp是设置为(GL_KEEP, GL_KEEP, GL_KEEP)的，
            所以不论任何测试的结果是如何，模板缓冲都会保留它的值。
            默认的行为不会更新模板缓冲，所以如果你想写入模板缓冲的话，你需要至少对其中一个选项设置不同的值。
         
         
         */
        
        glDisable(GL_DEPTH_TEST); // 禁用了深度测试，让放大的箱子，即边框，不会被地板所覆盖
        shaderSingleColor.use();
        
        
        float scale = 1.1f;         // 不用沿着法线方向，而是把整个模型放大;;   关闭了模板写入,但是还是会做模板测试
        // cubes
        //glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glBindVertexArray(cubeVAO);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        model = glm::mat4(1.0f);
        
        model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(scale, scale, scale)); // !!! 如果物体的中心不是几何中心, 可能会导致缩放不对; 严格来说应该按照 ”都往模型的法线方向增长一丢丢“
        shaderSingleColor.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        
        //glStencilFunc(GL_NOTEQUAL, 2, 0xFF); // 是不等于比较 所以会有问题
        model = glm::mat4(1.0f);
        model = glm::rotate(model, 3.14f / 4.0f , glm::vec3(0.0,1.0,0.0));
        model = glm::scale(model, glm::vec3(scale, scale, scale));
        model = glm::translate(model, glm::vec3(-0.1f, 0.0f, 0.0f));
        shaderSingleColor.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        
        glBindVertexArray(0);
        glStencilMask(0xFF); // 恢复默认状态
        glStencilFunc(GL_ALWAYS, 0, 0xFF); // 恢复默认状态
        glEnable(GL_DEPTH_TEST);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &cubeVBO);
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
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
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
