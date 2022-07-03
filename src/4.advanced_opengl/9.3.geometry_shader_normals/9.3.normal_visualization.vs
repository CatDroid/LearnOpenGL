#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

// 需要同时修改 gs和vs
 
out VS_OUT
{
    vec4 normalClip;
    vec4 positionClip;
    vec3 normal;
} vs_out;

uniform mat4 view;
uniform mat4 model;
uniform mat4 projection;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    
  
    //vs_out.normal = vec3(projection * vec4(normalMatrix * aNormal, 0.0));
    /*
        外面的vec3 会把 projection * vec4(normalMatrix * aNormal, 0.0) 计算得到齐次坐标系的w给直接去掉了，
        而一般的透视之后，Wclip = -Zeye (clip和eye是下标，分别代表裁剪坐标系和视图坐标系), 也就是齐次坐标的w不能直接去掉。

        总的结论是：如果要在裁剪空间(clip space)中做处理，就要保证在几何着色器(gs)中,
        gl_in[index].gl_Position 和 gs_in[index].normal 都是裁剪空间的齐次坐标。
     */
    //vs_out.normalClip = vec4(vec3(projection * vec4(normalMatrix * aNormal, 0.0)),0.0); // 打开这个 看到旧代码的错误
    vs_out.normalClip = projection * vec4(normalMatrix * aNormal, 0.0);
    vs_out.positionClip = projection * view * model * vec4(aPos, 1.0);
 
    vs_out.normal = vec3(vec4(normalMatrix * aNormal, 0.0)); // 同样法线也转换到view space
    gl_Position = view * model * vec4(aPos, 1.0);
 
    
	// 注意这里还没有投影, 不是clip space, 是view space 
    // 后面有几何着色器gs, 所以也可以不写入 gl_Position, 而是写入自己定义的变量varying
    // 
    // 重要的是最终 光栅化着色器阶段 之前输出到 gl_Position 的内容(vs输出或者gs输出)
    // 这个必须是 齐次剪辑空间 的坐标 
    // https://stackoverflow.com/questions/6529263/glsl-geometry-shaders-and-projection-matrices

}
