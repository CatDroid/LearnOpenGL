#version 330 core
layout (location = 0) in vec3 aPos;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

// 基准偏移量 和 对齐偏移量(相对于块开头的偏移,并且一定是基准偏移量整数倍) 

// std140 按照固定规则 
// 注意 "数组" 不管是标量或向量, 每个元素的   基准对齐量, 都是4N(vec4,16个字节) , 比较耗内存
//        "矩阵"                           每个列向量的基准对齐量, 都是4N(vec4,16个字节)
// shared 不同的program的uniform都是一样的排序, 但是按照硬件最优化方式
// packed 不同的program的uniform排序不一样, shared/packed都要用glGetUniformIndices查询

uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}  