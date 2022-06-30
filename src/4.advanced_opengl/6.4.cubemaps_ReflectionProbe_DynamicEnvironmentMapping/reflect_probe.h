//
//  reflect_probe.h
//  LearnOpenGL
//
//  Created by hohanloong on 2022/6/29.
//

#ifndef reflect_probe_h
#define reflect_probe_h

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


class ReflectProbe
{
private:
    GLuint probeCubeTexId = 0;
	GLuint probeCubeFboIds[6] = { 0 };
    GLuint probeCubeRboId = 0;
    int fboSize = 0;
    
    // camera
    Camera probeCamera{ glm::vec3(0.0f, 0.0f, 0.0f) };
    
public:
    ReflectProbe(int textureSize)
    {
        fboSize = textureSize;
        
        glGenTextures(1, &probeCubeTexId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, probeCubeTexId);
        for (unsigned int i = 0; i < 6; i++)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, textureSize, textureSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        

        // fbo 和 rbo 深度
        glGenRenderbuffers(1, &probeCubeRboId);
        glBindRenderbuffer(GL_RENDERBUFFER, probeCubeRboId);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, textureSize, textureSize);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        
        //  只生成一个 每次切换 color附件
        glGenFramebuffers(6, probeCubeFboIds);

		int side = 0;
		for (auto& probeCubeFboId : probeCubeFboIds)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, probeCubeFboId);

			// GL_TEXTURE_2D 普通2D纹理
			// GL_TEXTURE_2D_MULTISAMPLE 2D多重采样纹理
			// GL_TEXTURE_CUBE_MAP_POSITIVE_X cubemap的某个面的纹理 （注意: !!! 纹理ID是同一个!!!)
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (side++), probeCubeTexId, 0); // 暂时先绑定到cubemap的x面
																															 //glFramebufferTexture2D(probeCubeFboId, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, probeCubeRboId , 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, probeCubeRboId);
			// 注意!! 附件是rbo, 绑定不是 glFramebufferTexture2D  !!
			// glFramebufferRenderbuffer renderTarget 必须是 GL_RENDERBUFFER. 
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! " << status << endl;
			}

		}
      
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // 没有模型 不用建VBO VAO等
        
        
        
    }
    
    void DrawSceneToCubemap( void(*_drawScene)(glm::mat4& view, glm::mat4& projection, glm::vec3& cameraPos, unsigned int _) )
    {
        // camera可以调整焦距 或者 fov 来实现 缩放
        
        // 焦距是zNear 固定是0.1f
        // fov一般是 通过垂直fov+radio来实现, 加上zNear, 这三者可以限制了水平fov (相当于这4个因素，有3个自由度)
        
	 
		glViewport(0, 0, fboSize, fboSize);

        float radio = fboSize / fboSize ; // == 1.0
        float fov   = 360.0 / 4.0;    // 90.0度∫
        glm::mat4 projection = glm::perspective(glm::radians(fov), radio, 0.1f, 100.0f);
        
		static float s_Side[] = { 
			GL_TEXTURE_CUBE_MAP_POSITIVE_X, // 右边 
			GL_TEXTURE_CUBE_MAP_NEGATIVE_X ,						
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // cubemap z+轴 往上看 
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // 前面  cubemap z+轴    
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 
		};
        //static float s_Yaws[]   = {90, -90,   0,     180,     90,  -90}; // 航向角(水平角)
        //static float s_Pitchs[] =  {  0,     0,  0,  0,   -90,  90};     // 俯仰角
        
		// 按照世界坐标系(右手) 跟cubemap左手不同
		static glm::vec3 s_Fronts[] = { glm::vec3{1.0,0.0,0.0}, glm::vec3{-1.0,0.0,0.0}, glm::vec3{0.0, 1.0, 0.0}, glm::vec3{0.0,-1.0, 0.0}, glm::vec3{0.0,0.0,-1.0}, glm::vec3{0.0,0.0,1.0}};
		static glm::vec3 s_Ups[]    = { glm::vec3{0.0,1.0,0.0}, glm::vec3{ 0.0,1.0,0.0}, glm::vec3{0.0, 0.0, 1.0}, glm::vec3{0.0, 0.0, -1.0}, glm::vec3{0.0,1.0,0.0}, glm::vec3{0.0,1.0, 0.0} };
		//static glm::vec3 Rights   = { glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{}, glm::vec3{}  };


        for (int i = 0 ; i < 6 ; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, probeCubeFboIds[i]);
			// windows上不能这样换的?  但是RenderDoc抓帧是正常的 
            //glFramebufferTexture2D(probeCubeFboId, GL_COLOR_ATTACHMENT0, s_Side[i], probeCubeTexId ,0); 
			glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // 不用加载 原来颜色附件数据从fbo到tile buffer
            	
			probeCamera.Front = s_Fronts[i];
			probeCamera.Up    = s_Ups[i];

            //probeCamera.Yaw   = s_Yaws[i] ;
            //probeCamera.Pitch = s_Pitchs[i];
            // probeCamera.ProcessMouseMovement(0, 0, false);
            
            // 渲染当前probe位置物体 以外的物体
            auto view = probeCamera.GetViewMatrix();
            _drawScene(view, projection, probeCamera.Position, 0);
            
            //if (glInvalidateFramebuffer != nullptr)
            //{
            //    static GLenum discardFbo[] = {GL_DEPTH_STENCIL_ATTACHMENT};
            //    glInvalidateFramebuffer(GL_FRAMEBUFFER, 1, discardFbo); // 深度末班附件不用写入
            //}
         
            
            /*
                glInvalidateFramebuffer()是OpenGL ES 3.0新增的标准API接口
                在OpenGL ES 2.0上功能相似的extension为 EXT_discard_framebuffer。
            
                调用glBindFramebuffer() 激活待渲染的frame buffer;
                调用glInvalidateFramebuffer() 设置无需保留数据的attachment，此处为color attachment 0和color attachment 1;
                设置渲染状态，下发渲染命令。
             
                如果在glBindFramebuffer和glDrawArrays之间没有 glClear 或 glInvalidateFramebuffer 的调用，
                GPU会从DDR上读取render target的数据到内部tile buffer上，作为渲染的初始状态。
                这样就产生了额外的GPU读带宽。
             
             
                某渲染场景仅包含两个render pass，
                0th render pass的color render target渲染结果在1th render pass中作为纹理贴图使用，
                depth/stencil attachment在 0th render pass结束后没有被用到。
                因此，0th render pass的depth/stencil attachment数据不用从GPU Tile buffer上写出到DDR
             
                OpenGL ES 2.0上extension EXT_discard_framebuffer提供的API
                <OpenGLES/ES2/glext.h>
                void DiscardFramebufferEXT(enum target, // FRAMEBUFFER
                    sizei numAttachments,
                    const enum *attachments);           // 指定哪些附件不用写入
             
             */
            
            
        }

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
       
    }
    
    int GetReflectProbeTexture()
    {
        return probeCubeTexId; // 作为其他物体渲染的cubemap环境贴图(反射或者折射)
    }
    
};



#endif /* reflect_probe_h */
