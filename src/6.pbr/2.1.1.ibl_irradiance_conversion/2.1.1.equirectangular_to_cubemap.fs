#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

/*
 EquiRectangular  等距矩形
 
 Equirectangular projection(ERP)是一种简单的投影方式，
 将经线映射为恒定间距的垂直线，将纬线映射为恒定间距的水平线。
 这种投影方式映射关系简单，但既不是等面积的也不是保角的，引入了相当大的失真
 
 投影既不等面积也不保角。由于这种投影引入的变形，它在导航或地籍制图中几乎没有用处，主要用于专题制图。
 
 横轴：赤道
 纵轴：中央经线
 坐标原点：中央经线与赤道的交点，用O表示。赤道以南为负，以北为正；中央经线以东为正，以西为负
 
 全球分为二十四个时区
 以能够被15整除的经度作为该区域的中央子午线
 每一时区占经度15度
 本初子午线-0度经线，亦称格林尼治子午线
 本初子午线的东西两边分别定为东经和西经
 
 
 */

uniform sampler2D equirectangularMap;

// https://blog.csdn.net/lin453701006/article/details/76919188
//
//
// float PI=3.14152
// vec2 invAtan = vec2(0.1591, 0.3183) = vec2(1/(2*PI), 1/PI); =  1/2π  1/π
//
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) // v是立方体的 模型坐标系的坐标  用作 “球面坐标系的坐标”
{
    //  笛卡尔坐标系--球面极坐标
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y)); //  φ(水平)  从-180到180   θ(垂直) 从-90到90 (v是单位向量)
    uv *= invAtan;  // 归一化 φ= -0.5到0.5  θ=-0.5到0.5
    uv += 0.5;      //       φ= 0 到 1     θ= 0 到 1
    return uv;
    
    // 这里把x轴和y轴所在平面 作为 φ=0 往z轴正方向作为φ的正方向 (可能有左右镜像)
}

void main()
{		
    vec2 uv = SampleSphericalMap(normalize(WorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}
