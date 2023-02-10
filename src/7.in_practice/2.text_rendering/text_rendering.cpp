#include <iostream>
#include <map>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<wchar_t, Character> Characters;
unsigned int VAO, VBO;

#include <string>
#include <codecvt>

std::wstring s2ws(const std::string& str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

int main()
{

    std::cout << "hello" << " len:" << sizeof("hello") << std::endl;         // 窄多字节字符串字面量。无前缀字符串字面量的类型是 const char[N]，其中 N 是以执行窄编码的编码单元计的字符串的大小，包含空终止符

    std::wcout << u8"hello" << " len:" << sizeof(u8"hello") << std::endl;// UTF-8 编码的字符串字面量。u8"..." 字符串字面量的类型是 const char[N] (C++20 前)const char8_t[N] (C++20 起)，其中 N 是以 UTF-8 编码单元计的字符串的大小，包含空终止符。

    std::cout << u"hello" << " len:" << sizeof(u"hello") << std::endl;   // len=12=6*2  UTF-16 编码的字符串字面量。u"..." 字符串字面量的类型是 const char16_t[N]，其中 N 是以 UTF-16 编码单元计的字符串的大小，包含空终止符。
    std::cout << U"hello" << " len:" << sizeof(U"hello") << std::endl;  // len=24=6*4 UTF-32 编码的字符串字面量。U"..." 字符串字面量的类型是 const char32_t[N]，其中 N 是以 UTF-32 编码单元计的字符串的大小，包含空终止符。
    std::wcout << L"hello" << " len:" << sizeof(L"hello") << std::endl; // 宽字符串字面量。L"..." 字符串字面量的类型是 const wchar_t[N]，其中 N 是以执行宽编码的编码单元计的字符串的大小，包含空终止符。

    std::cout << R"("he\\llo)" << " len:" << sizeof(R"("he\\llo)") << std::endl;      //原始字符串字面量。       R 用于避免转义任何字符。 分隔符间的任何内容都成为字符串的一部分。若存在 前缀 则具有与上述相同的含义。
    std::wcout << LR"("he\\llo)" << " len:" << sizeof(LR"("he\\llo)") << std::endl;//原始字符串宽字符字面量。R 用于避免转义任何字符。分隔符间的任何内容都成为字符串的一部分。若存在 前缀 则具有与上述相同的含义。

    // wchar_t是C/C++的字符类型，是一种扩展的存储方式。 GNU Libc规定wchar_t为32位
    // wchar_t类型主要用在国际化程序的实现中，wchar_t是Unicode字符的数据类型
    // unicode编码的字符一般以wchar_t类型存储
    std::wstring ustr = s2ws(std::string("中文"));
    
    std::cout << ustr[0] << std::endl; // '中'  unicode 是 \u4e2d  也就是 20,013
    std::cout << ustr[1] << std::endl; // '文'  unicode 是 \u6587  也就是 25,991
   
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

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // compile and setup the shader
    // ----------------------------
    Shader shader("text.vs", "text.fs");
    // 直接使用正交投影矩阵(不管zNear和zFar), 这样物体的位置(顶点坐标) 可以直接用屏幕像素坐标(直接是视图空间的坐标)
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

	// find path to font  字体库 
    std::string font_name = FileSystem::getPath("resources/fonts/LXGWWenKai-Regular.ttf");
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }
	
	// load font as face  将字体加载为"Face" ?  一个 FreeType face 包含一组字形 glyphs
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as  字形的宽和高 宽为0让face根据高动态计算宽 
        FT_Set_Pixel_Sizes(face, 0, 48);  

        // disable byte-alignment restriction  关闭字节对齐
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // 源文件要使用utf-8编码
        std::wstring ustr = s2ws(std::string("预加载的字形OpenGL中文显示, 这是包含了所有要显示的字形演示实例"));

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < ustr.size() ; c++)
        {
            //   typedef unsigned long  FT_ULong;
            
            

            // Load character glyph  把其中一个字形设置为 活动字形
            if (FT_Load_Char(face, ustr[c], FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width, // 字形的宽和高 刚好是 纹理的大小 
                face->glyph->bitmap.rows,  // 没有height只有rows作为高  
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = { // 一个字符串生成一个png图片 
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows), // 字形的宽高 刚好实际文字的开始和结束
                glm::ivec2(face->glyph->bitmap_left,   face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x) // 单位是1/64pixels 所以是比较大的数字
            };
            Characters.insert(std::pair<wchar_t, Character>(ustr[c], character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    
    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        RenderText(shader, "预加载的字形",              25.0f, 25.0f,     1.0f,    glm::vec3(0.5, 0.8f, 0.2f));
        RenderText(shader, "中文字形示例OpenGL",  540.0f, 570.0f,  0.5f,    glm::vec3(0.3, 0.7f, 0.9f));
       
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
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// render line of text
// -------------------
void RenderText(Shader &shader, std::string text, float x, const float y, float scale, glm::vec3 color)
{
    // x和y是这段文字的起始位置   y是基线

    // activate corresponding render state	
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::wstring ws =  s2ws(text);
    
    for (auto itor = ws.begin(); itor != ws.end(); itor++)
    {
        Character ch = Characters[*itor];

        // x是以advance前进 
        float xpos = x + ch.Bearing.x * scale; //  ch.Bearing.x 字形离原点的距离 
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale; // ch.Size.y是字形的height  ch.Bearing.y是字形在base基线上的高度

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        /*
          ypos + h       0(3)           (5)
                            
                            
           ypos            1               2(4)
                           xpos          xpos + w

           逆时针 
        */

        // update VBO for each character
        float vertices[6][4] = {
            { xpos,       ypos + h,     0.0f, 0.0f },    // 顶点坐标xy,  纹理坐标zw        
            { xpos,        ypos,          0.0f, 1.0f },
            { xpos + w, ypos,          1.0f, 1.0f },

            { xpos,        ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,         1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)  advance 是 1/64 像素的数量
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
