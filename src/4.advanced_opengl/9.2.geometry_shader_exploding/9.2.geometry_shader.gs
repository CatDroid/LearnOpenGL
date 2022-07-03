#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 6) out; // 修改输出顶点数目

in VS_OUT 
{
    vec2 texCoords;
    vec4 positionViewSpace;
} gs_in[];

out vec2 TexCoords;
out vec3 fColor ;

uniform mat4 projection;
uniform float time;

vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude; // 幅度是 0~magnitude的正弦波
    return position + vec4(direction, 0.0); // 沿着法线 移动 0~magnitude
}

vec4 explodeViewSpace(vec4 position, vec3 normal)
{
    float magnitude = 0.5; // ViewSpace 会比较大??
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude;
    return position + vec4(direction, 0.0);
}

vec3 GetNormal()
{
    // 每一个点的法线值和面的法线值是不一样的
    // 所以不用传入顶点的法线, 而是直接用三角形, 面法线, 沿着面法线, 往外移动三角形
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross( a, b)); // 这里的顺序很重要  注意这个是clipSpace的gl_Position ! 并且这里直接去掉了w分量 ??
}

vec3 GetNormalViewSpace()
{
    // 实际上 positionViewSpace 的w都是1
    vec3 a = vec3(gs_in[0].positionViewSpace) - vec3(gs_in[1].positionViewSpace);
    vec3 b = vec3(gs_in[2].positionViewSpace) - vec3(gs_in[1].positionViewSpace);
    return normalize(cross(b, a)); // CCW  右手
}

void main() 
{
    #if 1
    fColor = vec3(1.0, 0.5, 0.5); // 红色是clipspace
    //fColor = vec3(1.0, 1.0, 1.0);
    vec3 normal = GetNormal();

    gl_Position =  explode(gl_in[0].gl_Position, normal);
    TexCoords = gs_in[0].texCoords;
    EmitVertex();

    gl_Position = explode(gl_in[1].gl_Position, normal);
    TexCoords = gs_in[1].texCoords;
    EmitVertex();

    gl_Position = explode(gl_in[2].gl_Position, normal);
    TexCoords = gs_in[2].texCoords; // !! 注意我们在发射顶点之前输出了对应的纹理坐标
    EmitVertex();

    EndPrimitive();
    #endif
    
    #if 1
    fColor = vec3(0.5, 1.0, 0.5); // 绿色是viewspace
    vec3 normalViewSpace = GetNormalViewSpace();
    
    gl_Position = projection * explodeViewSpace(gs_in[0].positionViewSpace, normalViewSpace);
    TexCoords = gs_in[0].texCoords;
    EmitVertex();

    gl_Position = projection * explodeViewSpace(gs_in[1].positionViewSpace, normalViewSpace);
    TexCoords = gs_in[1].texCoords;
    EmitVertex();

    gl_Position = projection * explodeViewSpace(gs_in[2].positionViewSpace, normalViewSpace);
    TexCoords = gs_in[2].texCoords; // !! 注意我们在发射顶点之前输出了对应的纹理坐标
    EmitVertex();
    #endif
    
    EndPrimitive();
}
