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
       ???????????????(Nanosuit)?????????????????????????????????.obj??????????????????.mtl?????????.mtl????????????????????????????????????????????????????????????
       ???blender??????
     
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
     
        nanosuit.mtl                        -- ????????????????????? (MTL) ??? .MTL ??????????????? .OBJ ?????????????????????
                                            -- ????????? Wavefront Technologies ??????
                                            -- ??????????????????????????? .OBJ ????????????????????????????????????????????????
                                            -- OBJ ??????????????????????????? .MTL ?????????????????????????????????????????????????????????????????????????????????????????????
                                            -- ????????? Phong ????????????, ??????????????????????????????, ?????????????????????
     
                                            -- MTL ?????????????????????????????????????????????????????????????????????"????????????"???"????????????"??????????????????
                                                ?????????????????????????????????????????????????????????????????????????????????????????? MTL ?????????????????????
     
     
             # Blender MTL File: 'nanosuit.blend'
             # Material Count: 6

             newmtl Arm                    -- ?????? .mtl ????????????????????????????????? ????????????????????????????????????????????????????????? newmtl ???????????????
             Ns 96.078431                  -- ?????? ???????????????????????? Ns   ranges between 0 and 1000
             Ka 0.000000 0.000000 0.000000 -- ??????????????????????????? Ka ????????? ??????????????? RGB ???????????????????????????????????? 0 ??? 1 ?????????
             Kd 0.640000 0.640000 0.640000 -- # Ks 0.000 0.000 0.000 ????????? black (off)
             Ks 0.500000 0.500000 0.500000
             Ni 1.000000                    -- optical density ????????????????????????????????????????????? ????????????????????????
                                            -- ????????????????????? 0.001 ??? 10???
                                                ??? 1.0 ??????????????????????????????????????????
                                                ???????????????????????????????????? ???????????????????????? 1.5??? ?????????????????? n1 * sin??1 = n2 * sin??2
                                                ?????? 1.0 ????????????????????????????????????????????????  ????????????????????
     
             d 1.000000                     -- ????????????????????????( transparent)??????????????? ??????(dissolved)??? ??????
                                            -- ???????????????????????????(real transparency)????????????????????????????????????(thickness)???
                                            -- ???d?????????????????? 1.0????????????????????????(fully opaque)?????? Tr???????????? 0.0 ??????  ???Tr???Dissolve????????????
                                            -- ??????(Dissolve) ??????????????????????????????
                                                     # ?????????????????? 'd'
                                                     d 0.9
                                                     # ?????????????????? 'Tr' (inverted: Tr = 1 - d)
                                                     Tr 0.1
             illum 2
     
                                            -- ???????????????????????????????????????????????????
                                            -- ????????????????????????????????????d?????????Tr????????????????????????????????????????????????
                                            -- ???????????????????????????????????????????????????????????????????????????????????????????????? ????????????????????????
                                                     0. ???????????????????????????
                                                     1. Color on and Ambient on ???????????????????????????
                                                     2. Highlight on ????????????
                                                     3. Reflection on ??? Ray trace on
                                                     4. ????????????Glass on????????????Ray trace on
                                                     5. ?????????Fresnel on ??? Ray trace on
                                                     6. ????????????????????????????????????????????????????????????????????????
                                                     7. ????????????????????????????????????????????????????????????????????????
                                                     8. ?????????????????????????????????
                                                     9. ??????????????????????????????????????????????????????
                                                     10. ???????????????????????????????????????
     
 
     
             map_Bump arm_showroom_ddn.png
             map_Ka arm_showroom_refl.png
             map_Kd arm_dif.png
             map_Ks arm_showroom_spec.png    -- ???????????? Texture options
                                                    -bm mult_value                  # ??????????????? (for bump maps only)
                                                    -imfchan r | g | b | m | l | z   ????????????????????????????????????????????????????????????
                                                            -imfchan r
                                                            # says to use the red channel of bumpmap.tga as the bumpmap
                                                            bump -imfchan r bumpmap.tga
                                                    -type cube_top    | cube_bottom |      # ??????cubemap????????????????????????
                                                           cube_front | cube_back   |
                                                           cube_left  | cube_right
     
                                                    ?????????????????????????????????????????????????????????????????????????????????"????????????????????????"???
                                                    ??????????????????????????????????????????????????????
                                                    ????????????????????????????????????????????????????????????????????????
     
                                                    ?????? bump texbump.tga -bm 0.2 ????????? bump -bm 0.2 texbump.tga
     
                Physically-based Rendering PBR??????  --- ?????? 3D ????????????????????? Clara.io ???????????????????????? MTL ???????????????????????????????????????????????????????????????
                                                     Pr/map_Pr  #?????????
                                                     Pm/map_Pm  # ?????????
                                                     Ps/map_Ps  # ??????
                                                     Pc         # ????????????
                                                     pcr        # ???????????????
                                                     Ke/map_Ke  # ?????????
                                                     aniso      # ????????????
                                                     anisor     # ??????????????????
                                                     norm       # ???????????????RGB???????????????????????????XYZ?????????
     
     
             ....
     
       mtllib nanosuit.mtl ------ Material template library ???????????????
             ???????????????(polygons)????????????(visual aspects)???????????????????????? .mtl ????????????
             ????????? OBJ ??????????????????????????? MTL ??????????????? .mtl ??????????????????????????????????????????????????????
      
       o Visor       o [object name]  ???????????? ????????????  -- ?? ????????????mesh ???
                     o [object name]
                       ...
                       g [group name]  ??????????????
                       ...
 
       usemtl Glass  usemtl [material name] ????????????????????????????????????????????? ????????????????????? .mtl ??????????????????????????????????????????
     
       s 1          ???????????????????????????????????????????????? ?????
      
       ???
       f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...
       f v1//vn1 v2//vn2 v3//vn3 ...
      
       ???  ????????????l???????????? L??????????????????????????????????????? polyline ?????????????????????
       l v1 v2 v3 v4 v5 v6
    */
    // load models
    // -----------
    //Model ourModel(FileSystem::getPath("resources/objects/backpack/backpack.obj"));
    Model ourModel(FileSystem::getPath("resources/objects/nanosuit/nanosuit.obj"));
    
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
        
        
        
        // light properties  ????????????uniform--????????????
        //ourShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f); // ??????Ka Kd Ks?????? 
        //ourShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        //ourShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("light.direction", -0.0f, -0.0f, 1.0f); // ?????????????????? ???????????????????????? 
        

        // view/projection transformations ????????????uniform--VP??????
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        
        
        // camera postion  ????????????uniform--????????????
        ourShader.setVec3("viewPos", camera.Position);
        
        
        // render the loaded model ???????????????uniform??????
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        
        // Draw???????????? (??????????????? texture?????????uniform ??? VAO VEO???
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
