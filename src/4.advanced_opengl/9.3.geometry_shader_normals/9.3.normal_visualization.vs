#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out VS_OUT {
    vec3 normal;
} vs_out;

uniform mat4 view;
uniform mat4 model;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    vs_out.normal = vec3(vec4(normalMatrix * aNormal, 0.0)); // 同样法线也转换到view space

    gl_Position = view * model * vec4(aPos, 1.0); 
	// 注意这里还没有投影, 不是clip space, 是view space 
    // 后面有几何着色器gs, 所以也可以不写入 gl_Position, 而是写入自己定义的变量varying
    // 
    // 重要的是最终 光栅化着色器阶段 之前输出到 gl_Position 的内容(vs输出或者gs输出)
    // 这个必须是 齐次剪辑空间 的坐标 
    // https://stackoverflow.com/questions/6529263/glsl-geometry-shaders-and-projection-matrices

}