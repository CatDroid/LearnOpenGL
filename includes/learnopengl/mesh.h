#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/shader.h>

#include <string>
#include <vector>
using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
	//bone indexes which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE]; // 骨骼数目?? 4个 ???
	//weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

class Mesh {
public:
    // mesh Data
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<Texture>      textures;
    unsigned int VAO;
    
    glm::vec4 ka;
    glm::vec4 kd;
    glm::vec4 ks;
    float shininess = 0;

    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void Draw(Shader &shader) 
    {
        //
        // shader开发者也可以自由选择需要使用的数量，他只需要定义正确的采样器就可以了
        //（虽然定义少的话会有点浪费绑定和uniform调用）
        //
        // bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        unsigned int refectNr   = 1; // 反射贴图
        
        
        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding 纹理单元从 GL_TEXTURE0 开始, 对应uniform从0开始
            // retrieve texture number (the N in diffuse_textureN)
            string number;
            string name = textures[i].type;
            
            //
            // shader中Sampler2D应该按照 texture_diffuse1 texture_normal1 等命名方式
            // material.texture_diffuse1  material.texture_diffuse2 (从1开始,但是纹理单元从0开始)
            //
            // 注意跟 Model.processMesh 要保持一致
            //
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++); // transfer unsigned int to string
            else if(name == "texture_normal")
                number = std::to_string(normalNr++); // 法线贴图, assimp不兼容obj的bumpMap 所以用 aiTextureType_HEIGHT 替换
             else if(name == "texture_height")
                number = std::to_string(heightNr++); // ???? 这个assimp加载obj用什么?? obj没有高度贴图 ??
            else if(name == "texture_refl")
                number = std::to_string(refectNr++); // 反射贴图, assimp不支持， 所以用 aiTextureType_AMBIENT 替换
            
            //
            // shader中的采样器 绑定到 纹理单元(??这样不是绑定到采样器,所以用纹理的参数来filter和wrapper???)
            // now set the sampler to the correct texture unit
            //
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }
        
   
        glUniform3f(glGetUniformLocation(shader.ID, "Ka"),  ka.x, ka.y, ka.z );
        glUniform3f(glGetUniformLocation(shader.ID, "Kd"),  kd.x, kd.y, kd.z );
        glUniform3f(glGetUniformLocation(shader.ID, "Ks"),  ks.x, ks.y, ks.z );
        glUniform1f(glGetUniformLocation(shader.ID, "shininess"), shininess);
        
        
        
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // render data 
    unsigned int VBO, EBO;

    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        
        // C++结构体有一个很棒的特性，它们的内存布局是连续的(Sequential)。 ??? 应该是有对齐 ???
        // structs 的一大优点是它们的内存布局对于它的所有项目都是连续的。
        // 效果是我们可以简单地传递一个指向结构的指针，它完美地转换为 glm::vec3/2 数组，该数组再次转换为 3/2 浮点数，后者转换为字节数组。
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // 指定了索引buffer 这样glDrawElements不用传buffer参数,而是根据VAO中 GL_ELEMENT_ARRAY_BUFFER 绑定的ebo
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal)); // offsetof 结构体成员的偏移
        // vertex texture coords
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords)); // sizeof 每个顶点的对齐
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        
        
		// ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));           // ??? 骨骼id ???

		// weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights)); // ??? 骨骼权重 ???
        
        
        glBindVertexArray(0);
    }
};
#endif
