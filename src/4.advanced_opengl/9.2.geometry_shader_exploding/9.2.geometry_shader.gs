#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT 
{
    vec2 texCoords;
} gs_in[];

out vec2 TexCoords; 

uniform float time;

vec4 explode(vec4 position, vec3 normal)
{
    float magnitude = 2.0;
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * magnitude; // 幅度是 0~magnitude的正弦波
    return position + vec4(direction, 0.0); // 沿着法线 移动 0~magnitude
}

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross( a, b)); // 这里的顺序很重要  注意这个是clipSpace的gl_Position !
}

void main() 
{    
    vec3 normal = GetNormal(); // 不传入法线, 而是直接用三角形, 面法线 

    gl_Position = explode(gl_in[0].gl_Position, normal);
    TexCoords = gs_in[0].texCoords;
    EmitVertex();

    gl_Position = explode(gl_in[1].gl_Position, normal);
    TexCoords = gs_in[1].texCoords;
    EmitVertex();

    gl_Position = explode(gl_in[2].gl_Position, normal);
    TexCoords = gs_in[2].texCoords; // !! 注意我们在发射顶点之前输出了对应的纹理坐标
    EmitVertex();

    EndPrimitive();
}
