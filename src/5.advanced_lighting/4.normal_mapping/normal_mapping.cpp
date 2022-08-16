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
unsigned int loadTexture(const char *path);
void renderQuad();

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

    // build and compile shaders
    // -------------------------
    Shader shader("4.normal_mapping.vs", "4.normal_mapping.fs");

    // load textures 注意没有设置 stbi 垂直镜像
    // -------------
    unsigned int diffuseMap = loadTexture(FileSystem::getPath("resources/textures/brickwall.jpg").c_str());
    unsigned int normalMap  = loadTexture(FileSystem::getPath("resources/textures/brickwall_normal.jpg").c_str());

    // shader configuration
    // --------------------
    shader.use();
    shader.setInt("diffuseMap", 0);
    shader.setInt("normalMap", 1);

    // lighting info
    // -------------
    glm::vec3 lightPos(0.5f, 1.0f, 0.3f);

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

        // configure view/projection matrices
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        // render normal-mapped quad
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians((float)glfwGetTime() * -10.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))); // rotate the quad to show normal mapping from multiple directions
        shader.setMat4("model", model);
        shader.setVec3("viewPos", camera.Position);
        shader.setVec3("lightPos", lightPos);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMap);
        renderQuad();

		// 在光源位置  姿态固定(平躺着)
        // render light source (simply re-renders a smaller plane at the light's position for debugging/visualization)
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos); 
        model = glm::scale(model, glm::vec3(0.1f));
        shader.setMat4("model", model);
        renderQuad();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// renders a 1x1 quad in NDC with manually calculated tangent vectors
// ------------------------------------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
	/* 
	Assimp的使用:

		切线:
			const aiScene* scene = importer.ReadFile
				(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace );
			aiProcess_CalcTangentSpace  Assimp会为每个加载的顶点计算出柔和的切线和副切线向量

			vector.x = mesh->mTangents[i].x;
			vector.y = mesh->mTangents[i].y;
			vector.z = mesh->mTangents[i].z;
			vertex.Tangent = vector;

		法线贴图:
			wavefront的模型格式（.obj）导出的法线贴图有点不一样，
			Assimp的aiTextureType_NORMAL并不会加载它的法线贴图, 而aiTextureType_HEIGHT却能
			当然，对于每个模型的类型和文件格式来说都是不同的。
			vector<Texture> specularMaps = 
				this->loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");

		备注:
		    旧版本assimp
			aiProcess_CalcTangentSpace并不能总是很好的工作也很重要。
			计算切线是需要根据纹理坐标的，
			有些模型制作者使用一些纹理小技巧，比如镜像一个模型上的纹理表面时，??? 也镜像了另一半的纹理坐标；
			这样当不考虑这个镜像的特别操作的时候（Assimp就不考虑）结果就不对了 

			法线也可能直接拷贝过去(?应该都会重新计算,得到该点的正确的模型坐标系的法线)
			
		纹理方向和TBN偏手性:

			无论如何，手性检查的想法是从切线和双切线计算法线， 然后将其与模型提供的法线进行比较。
			
			由于切线、双切线和法线都应该是正交的，如果你做交叉（切线，双切线），你应该得到法线。
			要比较两个向量(方向)是否相同，可利用两个相似向量的点积接近 1，并且相隔 180 度的两个向量为 -1 。
			所以，对于镜像的纹理坐标，叉积会给出一个与真实法线"相反"的法线向量；

			通过将点积, 与从叉积计算的法线 和 模型提供的法线相乘，镜像三角形将给出否定结果。

			显然，我们只需" 翻转切线的符号" 即可将镜像坐标系（具有镜像手性）转换回我们模型坐标系的正确手性。

			// TBN must form a right handed coord system.
			// Some models have symetric UVs. Check and fix.
			if (dot(cross(N, T), B) < 0.0)
				T = T * -1.0;

			-- hhl的理解: 
			 一个模型左右两边是对称, 法线会做了处理(正确), 纹理的增长方向两边是相反的
			 由于切线T和副切B的正方向是沿着纹理坐标增加的方向, 
			 这样T, B 和 N在一边是右手坐标系, 在另外一边就是左右坐标系??
			 ?? 但是顶点坐标也是镜像了?? 
			 ?? 所以要注意一下 模型给的法线 和  贴图坐标+纹理坐标计算是否一致??
    */

    if (quadVAO == 0)
    {
        // positions
        glm::vec3 pos1(-1.0f,  1.0f, 0.0f);	// 模型在XOY平面  就是竖着朝向镜头
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3( 1.0f, -1.0f, 0.0f);
        glm::vec3 pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);				// 左下角的顶点 对应 纹理的原点(左下角)
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);  
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);		// 模型坐标系下的法线 这里6个的顶点都一样

		// 每个表面都是一个三角形"平面"  
		// 三角形"平面" 可认为是切线和法线所在平面  TOB平面
		//
		// 法线贴图的切线和副切线与纹理坐标的两个方向对齐
		//
		// 在这个平面内,  该三角形纹理的方向(纹理坐标增长的方向), 跟T和B轴正方向一样
		
		// 拆解公式:
		// E = deltaU * T+ delatV * B
		//      = (U1 - U2) * T  +(V1 - V2)* B
		//      = (U1*T +V1*B) - (U2*T +V2*B) 
		//      = P1 - P2 
		//  P1 = (U1*T +V1*B)  = {T, B}(U1,V1)    ---U1和V1是个标量 (U1,V1)是纹理坐标系中的向量
		//  P2 = (U2*T +V2*B)                              ---T和B向量相当于在模型坐标系描述纹理坐标系的两个轴

		//  该公式描述的数学意义是，如何将一个点从uv空间映射到三维空间
		//  假设三角形中存在一点P，则向量OP=u（p）*T+v（p）*B，
		//  只要知道P点的uv坐标值，即可得到P点的三维坐标值
		//  也就是(U1,V1),(U2,V2)从纹理坐标系 转换到模型坐标系 
		//  现在相当于
		//      知道了 (U1,V1),(U2,V2),(U3,V3) 纹理坐标系中的位置, 
		//  同时也
		//       知道了模型坐标系中的位置P1 P2 P3 
		//  然后求T和B轴的坐标  
		// ?? 这样算出来的T和B轴是垂直的?? 讨论记录在 https://zhuanlan.zhihu.com/p/139593847

		// 
		//  注意(U,V)是二维坐标, 
		//      两个点所构成的方程组, 虽然有6个方程(6个参数) 
		//      但简单画图可以知道,  这两个点不能固定T轴和B轴在三维空间的位置(可以旋转起来)
		//      所以得到解析解,需要3个点   

		//  共享顶点 
		//		非共享情况: 
		//			只需为每个三角形计算一个切线/副切线，它们对于每个三角形上的顶点都是一样 
		//		共享情况:
		//			要注意的是大多数实现通常三角形和三角形之间都会共享顶点。
		//			!!! 这种情况下开发者通常将每个顶点的 ""法线和切线/副切线""等 ""顶点属性"" 平均化，
		//			以获得更加柔和的效果 (??这种情况?? 法线和切线都会做平均? 平均之后就不能相互垂直)
		//			!!! 这样切线就不一定跟法线垂直了, 需要做施密特正交化
		// 


        // calculate tangent/bitangent vectors of both triangles
        glm::vec3 tangent1, bitangent1;  // 第一个三角形平面 
        glm::vec3 tangent2, bitangent2;
        // triangle 1
        // ----------
        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

		// 逆矩阵 = 1/行列式 * 伴随矩阵 
        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		// tangent1 = glm::normalize(tangent1);
		// f*相当于一个标量乘上去了, 所以如果要normalize的话,也可以不用乘以f

        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
		// bitangent1 = glm::normalize(bitangent1);  

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		

        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

		// 顶点坐标(模型坐标系), 法线, 纹理坐标, 切线, 副切线(模型坐标系)
        float quadVertices[] = {
            // positions            // normal         // texcoords  // tangent                          // bitangent
            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
			// 两个三角形, 这里没有用共享顶点, 法线和切线没有做平均
            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
            pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
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
