#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <assimp/Importer.hpp>   // assimp Importer 导入器
#include <assimp/scene.h>        // assimp Assimp 加载模型至Assimp的一个叫做scene的数据结构 aiScene
#include <assimp/postprocess.h>  // assimp 模型导入后处理

#include <learnopengl/mesh.h>
#include <learnopengl/shader.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include "Resource.h"

using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        //
        // Importer aiProcess_Triangulate 这些都是assimp的接口
        // aiScene 是有assimp来维护 不能自己实例化或者销毁
        //
        // aiProcess_Triangulate        我们告诉Assimp，如果模型不是（全部）由三角形组成，它需要将模型所有的图元形状变换为三角形
        // aiProcess_FlipUVs            将在处理的时候翻转y轴的纹理坐标（你可能还记得我们在纹理教程中说过，在OpenGL中大部分的图像的y轴都是反的
        // aiProcess_GenNormals         如果模型不包含法向量的话，就为每个顶点创建法线
        // aiProcess_SplitLargeMeshes   将比较大的网格分割成更小的子网格，如果你的渲染有最大顶点数限制，只能渲染较小的网格，那么它会非常有用。
        // aiProcess_OptimizeMeshes     和上个选项相反，它会将多个小网格拼接为一个大的网格，减少绘制调用从而进行优化
        //
        // 后期处理指令 http://assimp.sourceforge.net/lib_html/postprocess_8h.html
        //
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        
        // check for errors 场景不完整 或者 没有根节点
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            // 如果遇到了任何错误，我们都会通过导入器的GetErrorString函数来报告错误并返回
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // 首先处理当前的节点，再继续处理该节点所有的子节点
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]]; // 每个node可能有多个mesh组成  mesh具体保存在scene中
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }
    
    
    std::string vec4ToString(glm::vec4 v4)
    {
        stringstream ss;
        ss << "{" << v4.x << "," <<  v4.y << "," << v4.z << "," << v4.w << "}" ;
        return ss.str();
    }

    // 一个node可能有多个mesh  每个mesh有自己的材质
    // 返回自定义的mesh  一个mesh有自己的网格
    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        
        std::cout << "mesh name: " << mesh->mName.C_Str() << std::endl;
        
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // Part.1 包含了5个顶点属性:顶点坐标,法线,纹理坐标,切向,副法，每一个顶点用一个Vertex结构体储存
        
        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector;
            // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            
            // assimp储存数据 按照 mVertices mNormals mTextureCoords mTangents mBitangents 分开所有顶点属性存放的
            // 而自定义的Vertex是把每个顶点属性都包含在一个结构体中 -- SOA AOS的区别
            
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                //
                // Assimp允许一个模型在一个顶点上有最多8个不同的纹理坐标
                // 我们不会用到那么多，我们只关心第一组纹理坐标。
                //
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        
        // Part.2 索引(面/图元), 现在遍历网格的每个面（一个面是网格中的一个的三角形）并检索相应的顶点索引
        //                     Assimp定义了每个网格都有一个面(Face)数组.每个面代表了一个图元.
        //                     由于使用了aiProcess_Triangulate选项）它总是三角形
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++) // 每个面的索引数目 一般是3个?? 图元是三角形
                indices.push_back(face.mIndices[j]);        
        }
        
        // Part.3 获取这个网格mesh对应的材质参数:漫反射贴图, 高光贴图 , 法线贴图, 高度贴图
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];   // 网格材质索引  mesh->mMaterialIndex
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN
        
        
        /*
            LEGACY API MATERIALS
                Legacy 是指最初在 2000 年左右在规范中实施的材料。这些材料绝不能被删除，因为大多数引擎都支持它们。
         
         */
        if (material->GetTextureCount(aiTextureType_AMBIENT))
        std::cout << "aiTextureType_AMBIENT:          " <<material->GetTextureCount(aiTextureType_AMBIENT)      << std::endl; // Phong模型的环境光贴图 ambient lighting equation.
        if (material->GetTextureCount(aiTextureType_DIFFUSE))
        std::cout << "aiTextureType_DIFFUSE:          " <<material->GetTextureCount(aiTextureType_DIFFUSE)      << std::endl;
        if (material->GetTextureCount(aiTextureType_SPECULAR))
        std::cout << "aiTextureType_SPECULAR:         " <<material->GetTextureCount(aiTextureType_SPECULAR)     << std::endl; // Phong模型的镜面贴图  或者 PBR Specular/Glossiness 高光光泽度贴图
        if (material->GetTextureCount(aiTextureType_NORMALS))
        std::cout << "aiTextureType_NORMALS:          " <<material->GetTextureCount(aiTextureType_NORMALS)      << std::endl; // 纹理是（切线空间）法线贴图。 同样，????切线空间法线贴图有"几个约定???"。 Assimp 确实（故意）不在这里区分。
        if (material->GetTextureCount(aiTextureType_HEIGHT))
        std::cout << "aiTextureType_HEIGHT:           " <<material->GetTextureCount(aiTextureType_HEIGHT)       << std::endl; // 按照惯例，较高的灰度值代表从基本高度开始的较高海拔。
        if (material->GetTextureCount(aiTextureType_SHININESS))
        std::cout << "aiTextureType_SHININESS:        " <<material->GetTextureCount(aiTextureType_SHININESS)    << std::endl; // 纹理定义了材质的光泽度。 光泽度实际上是镜面反射（phong）光照方程的指数。 ???通常会定义一个转换函数来将纹理中的线性颜色值映射到合适的指数???。
        if (material->GetTextureCount(aiTextureType_OPACITY))
        std::cout << "aiTextureType_OPACITY:          " <<material->GetTextureCount(aiTextureType_OPACITY)      << std::endl; // 纹理定义了每像素的不透明度。 通常“白色”表示不透明，“黑色”表示“透明”。 或者恰恰相反。
        if (material->GetTextureCount(aiTextureType_DISPLACEMENT))
        std::cout << "aiTextureType_DISPLACEMENT:     " <<material->GetTextureCount(aiTextureType_DISPLACEMENT) << std::endl; // 位移纹理?? 确切的目的和格式取决于应用程序。 较高的颜色值代表较高的顶点位移。
        if (material->GetTextureCount(aiTextureType_LIGHTMAP))
        std::cout << "aiTextureType_LIGHTMAP:        " <<material->GetTextureCount(aiTextureType_LIGHTMAP)      << std::endl; // 光照贴图纹理（又名环境光遮蔽） 此材质属性涵盖“光照贴图”和专用的“环境遮挡贴图”。 纹理包含像素最终颜色值的缩放值。 它的强度不受入射光的影响。
        if (material->GetTextureCount(aiTextureType_REFLECTION))
        std::cout << "aiTextureType_REFLECTION:      " <<material->GetTextureCount(aiTextureType_REFLECTION)   << std::endl; // 反射纹理  包含完美镜面反射的颜色。 很少使用，几乎从不用于实时应用程序。

      
        /*
            PBR Materials
                Maya 和其他建模包中的 PBR 定义现在使用此标准。这最初是在 2012 年左右引入的。Godot、Unreal 或 Unity3D 等游戏引擎对此提供了支持。
                使用它的建模包现在非常普遍。
        */
        
        if (material->GetTextureCount(aiTextureType_BASE_COLOR))
        std::cout << "aiTextureType_BASE_COLOR:               " << material->GetTextureCount(aiTextureType_BASE_COLOR)      << std::endl;
        if (material->GetTextureCount(aiTextureType_NORMAL_CAMERA))
        std::cout << "aiTextureType_NORMAL_CAMERA:            " << material->GetTextureCount(aiTextureType_NORMAL_CAMERA)   << std::endl;
        if (material->GetTextureCount(aiTextureType_EMISSION_COLOR))
        std::cout << "aiTextureType_EMISSION_COLOR:           " << material->GetTextureCount(aiTextureType_EMISSION_COLOR)  << std::endl;
        if (material->GetTextureCount(aiTextureType_METALNESS))
        std::cout << "aiTextureType_METALNESS:                " << material->GetTextureCount(aiTextureType_METALNESS)       << std::endl;  // 金属度
        if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS))
        std::cout << "aiTextureType_DIFFUSE_ROUGHNESS:        " << material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) << std::endl;// 粗糙度
        if (material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION))
        std::cout << "aiTextureType_AMBIENT_OCCLUSION:        " << material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) << std::endl; // AO环境遮挡
        
        /*
            PBR Material Modifiers
                一些现代渲染器具有更多的 PBR 修改器，
                这些修改器可能会覆盖在“基础”PBR 材质(the 'base' PBR materials)之上，以获得额外的真实感。
                这些使用了多个纹理贴图，所以只直接定义了基本类型
         */
        
        // sheen 关泽 -- 通常用于模拟覆盖有一层超细纤维的纺织品。 例如天鹅绒
        if (material->GetTextureCount(aiTextureType_SHEEN) )
        std::cout << "aiTextureType_SHEEN:        " << material->GetTextureCount(aiTextureType_SHEEN)           << std::endl;
        // 模拟 PBR 基材顶部的一层“抛光”或“漆”  Simulates a layer of 'polish' or 'laquer' layered on top of a PBR substrate
        if (material->GetTextureCount(aiTextureType_CLEARCOAT) )
        std::cout << "aiTextureType_CLEARCOAT:    " << material->GetTextureCount(aiTextureType_CLEARCOAT)       << std::endl;
        // 模拟表面透射，包括更多信息，例如壁厚  Simulates transmission through the surface May include further information such as wall thickness
        if (material->GetTextureCount(aiTextureType_TRANSMISSION) )
        std::cout << "aiTextureType_TRANSMISSION: " <<  material->GetTextureCount(aiTextureType_TRANSMISSION)   << std::endl;
        
    
        aiColor3D color;
        material->Get(AI_MATKEY_COLOR_AMBIENT, color);
        glm::vec4 Ka = glm::vec4(color.r, color.g, color.b, 1.0);
        material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        glm::vec4 Kd = glm::vec4(color.r, color.g, color.b, 1.0);
        material->Get(AI_MATKEY_COLOR_SPECULAR, color);
        glm::vec4 Ks = glm::vec4(color.r, color.g, color.b, 1.0);
        float shininess = 0;
        material->Get(AI_MATKEY_SHININESS, shininess);
        
        // 介质的折射率 n等于光在真空中的速度c 跟光在介质中的相速度v 之比 n=c/v  斯涅耳定律折射 n1 * sinθ1 = n2 * sinθ2
        float refractiveIndex = 1.0;
        material->Get(AI_MATKEY_REFRACTI, refractiveIndex);
        std::cout << "Phong---- Ka:" << vec4ToString(Ka) <<" Kd:" << vec4ToString(Kd) << " Ks:" << vec4ToString(Ks) << " shininess:" << shininess << " refractiveIndex:" << refractiveIndex << std::endl;
        
        
        // material properties
        //lightingShader.setFloat("material.shininess", 64.0f);
        

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());


        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        
        // 3. normal maps ??? 为什么这个type是 HEIGHT ??
        //        wavefront object format (.obj) 导出的法线贴图与 Assimp 的"约定" 略有不同，
		//        因为 aiTextureType_NORMAL 不加载法线贴图，而是 aiTextureType_HEIGHT
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        //std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        
        // 4. height maps ???? 这个是ambient ????
        //        Assimp并不支持反射贴图，
		//        我们可以使用环境贴图的方式将反射贴图从aiTextureType_AMBIENT类型中来加载反射贴图的材质  
		//        所以mtl文件中ka 是 map_Ka helmet_showroom_refl.png  refl是反射贴图 不是环境贴图
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_refl"); //"texture_height");
        //std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // 目前三个纹理图:
        // "use.jpg"   texture_diffuse    aiTextureType_DIFFUSE
        // "ular.jpg"  texture_specular   aiTextureType_SPECULAR
        // "al.png"    texture_normal     aiTextureType_HEIGHT
        
        
        // return a mesh object created from the extracted mesh data
        Mesh temp (vertices, indices, textures); // 每个mesh包含的三种数据 顶点 索引 纹理
        temp.ka = Ka;
        temp.kd = Kd;
        temp.ks = Ks ;
        temp.shininess = shininess;
        
        return temp;
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        // 纹理参数保存在 结构体 Texture
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            //
            // 每种功能类型纹理的个数 aiTextureType_DIFFUSE aiTextureType_SPECULAR aiTextureType_HEIGHT aiTextureType_AMBIENT
            
            /* 注意:
                  对于自带材质的fbx模型，内置材质是存在aiScene中的，所以要从aiScene读取内置材质aiTexture(也就是不用我们自己加载图片文件,因为图片在fbx中)
             
                    auto aitexture = scene->GetEmbeddedTexture(name.C_Str());
                    if (aitexture != nullptr)
                        tex.id = Resource::LoadTextureFromAssImp(aitexture, GL_CLAMP, GL_LINEAR, GL_LINEAR);
                    else
                        tex.id = Resource::LoadTexture(filePath.c_str(), GL_REPEAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
             
                  材质图片的绝对路径
                    mtl本身是一个文本文件，可以用记事本打开，
                    把一些贴图的绝对路径比如C://tex1.jpg这样的修改成tex1.jpg这样的相对路径，
                    然后把贴图和模型扔在同一个文件夹里应该就能正常读取了
            
            */
            
            // 获取某个功能类型纹理的 第i个 的路径str
            // 注意: brew install assimp 安装的话，需要把 /usr/local/include/assimp 更新到 LearnOpenGL/include/assimp/ 目录
            //      brew info assimp  assimp: stable 5.2.4 (bottled)
            //
            aiString str;
            mat->GetTexture(type, i, &str);
            
            std::cout << "name = " << str.C_Str() << "; type = " << typeName << std::endl;
            
            
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    // 优化 同一个路径的不加载两次
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory); // 纹理
                texture.type = typeName;   // 功能类型
                texture.path = str.C_Str();// 路径
                textures.push_back(texture);
                textures_loaded.push_back(texture);
                // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};


unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    /*
    unsigned int textureID;
    glGenTextures(1, &textureID);

    // 通过stbimage 解压图片文件  ??? 不支持压缩纹理 ???
    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1) // 只有一个分量 放到R通道  ??? 为什么不是A通道??
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        // 全部都是2D纹理
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
    */
    
    return Resource::LoadTexture(filename.c_str(), GL_REPEAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR, true);
}
#endif
