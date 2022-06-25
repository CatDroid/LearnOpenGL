#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
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

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");

    /*
       原版纳米装(Nanosuit)。这个模型被输出为一个.obj文件以及一个.mtl文件，.mtl文件包含了模型的漫反射、镜面光和法线贴图
       用blender生成
     
       https://en.wikipedia.org/wiki/Wavefront_.obj_file
     
        nanosuit.obj
     
            mtllib nanosuit.mtl
            ...
            o Helmet
            v 0.590436 15.014743 0.150037
            ...
            vt 0.153198 0.172363
            ...
            vn 0.738700 -0.110500 0.664900
            ...
            usemtl Helmet
            s 1
            f 9432/4940/9350 9433/4941/9350 9434/4942/9350
            ...
            o Body
            ...
     
        nanosuit.mtl                        -- 材质模板库格式 (MTL) 或 .MTL 文件格式是 .OBJ 的配套文件格式
                                            -- 同样由 Wavefront Technologies 定义
                                            -- 用于描述一个或多个 .OBJ 文件中对象的表面着色（材质）属性
                                            -- OBJ 文件引用一个或多个 .MTL 文件（称为“材料库”），并从那里按名称引用一个或多个材料描述。
                                            -- 并根据 Phong 反射模型, 定义表面的光反射属性, 用于计算机渲染
     
                                            -- MTL 格式虽然仍然被广泛使用，但已经过时，不完全支持"高光贴图"和"视差贴图"等后期技术。
                                                然而，由于格式的开放和直观的性质，这些可以很容易地通过自定义 MTL 文件生成器添加
     
     
             # Blender MTL File: 'nanosuit.blend'
             # Material Count: 6

             newmtl Arm                    -- 单个 .mtl 文件可以定义多种材料。 材料在文件中一个接一个地定义，每个都以 newmtl 命令开始：
             Ns 96.078431                  -- 权重 使用镜面反射指数 Ns   ranges between 0 and 1000
             Ka 0.000000 0.000000 0.000000 -- 材质的环境颜色使用 Ka 声明。 颜色定义在 RGB 中，其中每个通道的值介于 0 和 1 之间。
             Kd 0.640000 0.640000 0.640000 -- # Ks 0.000 0.000 0.000 意味着 black (off)
             Ks 0.500000 0.500000 0.500000
             Ni 1.000000                    -- optical density 材料也可以具有其表面的光密度。 这也称为折射率。
                                            -- 值的范围可以从 0.001 到 10。
                                                值 1.0 表示光在穿过对象时不会弯曲。
                                                增加光密度会增加弯曲量。 玻璃的折射率约为 1.5。 光的折射定律 n1 * sinθ1 = n2 * sinθ2
                                                小于 1.0 的值会产生奇怪的结果，不推荐使用  ???不高于真空??
     
             d 1.000000                     -- 材料可以是透明的( transparent)。这被称为 溶解(dissolved)。 ??????
                                            -- 与真正的透明度不同(real transparency)，结果不取决于对象的厚度(thickness)。
                                            -- “d”的默认值为 1.0，表示完全不透明(fully opaque)，跟 Tr”的值为 0.0 一样  ???Tr和Dissolve是什么???
                                            -- 溶解(Dissolve) 适用于所有照明模型。
                                                     # 有些实现使用 'd'
                                                     d 0.9
                                                     # 其他实现使用 'Tr' (inverted: Tr = 1 - d)
                                                     Tr 0.1
             illum 2
     
                                            -- 每种材料都有多种照明模型可供选择。
                                            -- 请注意，不需要为了使用“d”或“Tr”实现透明度而设置透明照明模型，
                                            -- 并且在现代使用中，通常不指定照明模型，即使使用透明材料也是如此。 照明模型列举如下
                                                     0. 颜色开启和环境关闭
                                                     1. Color on and Ambient on 开启颜色和开启环境
                                                     2. Highlight on 突出显示
                                                     3. Reflection on 和 Ray trace on
                                                     4. 透明度：Glass on，反射：Ray trace on
                                                     5. 反射：Fresnel on 和 Ray trace on
                                                     6. 透明度：折射开启，反射：菲涅耳关闭，光线追踪开启
                                                     7. 透明度：折射开启，反射：菲涅耳开启，光线追踪开启
                                                     8. 反射开启和光线追踪关闭
                                                     9. 透明度：玻璃开启，反射：光线追踪关闭
                                                     10. 将阴影投射到不可见的表面上
     
 
     
             map_Bump arm_showroom_ddn.png
             map_Ka arm_showroom_refl.png
             map_Kd arm_dif.png
             map_Ks arm_showroom_spec.png    -- 纹理选项 Texture options
                                                    -bm mult_value                  # 法线乘因子 (for bump maps only)
                                                    -imfchan r | g | b | m | l | z   指定文件的哪个通道用于创建标量或凹凸纹理
                                                            -imfchan r
                                                            # says to use the red channel of bumpmap.tga as the bumpmap
                                                            bump -imfchan r bumpmap.tga
                                                    -type cube_top    | cube_bottom |      # 使用cubemap的时候指定哪一边
                                                           cube_front | cube_back   |
                                                           cube_left  | cube_right
     
                                                    由于易于解析文件以及文件格式的非官方传播，文件可能包含"供应商特定的更改"。
                                                    根据规范，选项应该在纹理文件名之前。
                                                    但是，至少有一个供应商在最后生成带有选项的文件。
     
                                                    比如 bump texbump.tga -bm 0.2 而不是 bump -bm 0.2 texbump.tga
     
                Physically-based Rendering PBR选项  --- 在线 3D 编辑和建模工具 Clara.io 的创建者建议扩展 MTL 格式以包含以下参数来表示基于物理的渲染参数
                                                     Pr/map_Pr  #粗糙度
                                                     Pm/map_Pm  # 金属的
                                                     Ps/map_Ps  # 光泽
                                                     Pc         # 清漆厚度
                                                     pcr        # 清漆粗糙度
                                                     Ke/map_Ke  # 自发光
                                                     aniso      # 各向异性
                                                     anisor     # 各向异性旋转
                                                     norm       # 法线贴图（RGB分量代表表面法线的XYZ分量）
     
     
             ....
     
       mtllib nanosuit.mtl ------ Material template library 材料模板库
             描述多边形(polygons)视觉方面(visual aspects)的材料存储在外部 .mtl 文件中。
             可以从 OBJ 文件中引用多个外部 MTL 材料文件。 .mtl 文件可能包含一个或多个命名材料定义。
      
       o Visor       o [object name]  标签指定 命名对象  -- ?? 对应一个mesh ???
                     o [object name]
                       ...
                       g [group name]  对象分组??
                       ...
 
       usemtl Glass  usemtl [material name] 此标签指定其后元素的材质名称。 材料名称与外部 .mtl 文件中的命名材料定义相匹配。
     
       s 1          通过平滑组启用跨多边形的平滑着色 ?????
      
       面
       f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...
       f v1//vn1 v2//vn2 v3//vn3 ...
      
       线  以字母“l”（小写 L）开头的记录指定了构建折线 polyline 的顶点的顺序。
       l v1 v2 v3 v4 v5 v6
    */
    // load models
    // -----------
    Model ourModel(FileSystem::getPath("resources/objects/backpack/backpack.obj"));

    
    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

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
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
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
