#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));   
    vs_out.TexCoords = aTexCoords;
    
	// 当我们将它们与模型矩阵相乘时，它就是世界空间。 
	// 然后我们通过直接向 mat3 的构造函数提供相关的列向量来创建实际的 TBN 矩阵
	// 如果我们想要非常精确，我们会将 TBN 向量与法线矩阵相乘，因为我们只关心向量的方向
 
	// 在unity中法线的变换是用 法线变换矩阵 , 切线是用模型矩阵 
	// 这里都用 法线变换矩阵(切线也用?? 为什么不直接是 mat3(model) ？？)
	// aTangent/aNormal都是方向向量, 所以只要mat3x3 
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * aTangent); 
    vec3 N = normalize(normalMatrix * aNormal); 

	// 到这里, TBN三个向量是在 世界空间  的三个方向向量
	//vec3 lightDir  = TBN * normalize(lightPos - FragPos);
    //vec3 viewDir  = TBN * normalize(viewPos - FragPos);
	// !! 因为ps中只计算方向, 所以这里的变换都不包含位移 
 
	/*          
			如果 转换到具体位置, 就要  
			从切线到模型-->
			p(模型) = 
            |TBN(model),   aPos.xyz (某个顶点对模型) | * { p(切)， 1}
            | 0,                  1                                  |

			 模型-> 世界 
			 p(世界) =
             |  M     T |  * {p(模型), 1}                  
             |  0      1 |
			 (其中M=RS)

			 矩阵分块乘法
             | M*TBN      M*aPos + T    |      
             | 0              1                   |     

             M*aPos + T 就是模型坐标系的aPos点转换到世界坐标系下的坐标
                               --> 意味着 切线坐标系的原点，在世界坐标系中的描述 拿到了

			 如果TBN是世界坐标系, 那么偏移就要是这个顶点在世界坐标系中的偏移
			 | TBN(in World)   aPos(in World)|
			 | 0                      1                  |
			  
			 其逆是
             | inverse(M*TBN)   inverse(M*TBN) * -1 * (M*aPos + T ) |
             | 0                        1                                                      |

			 补充： 
			 对于变换 
			 p' = M*p + T
			        矩阵形式 
            {p',1} =  |M   T | * {p, 1}
                          |0   1  |
			 其逆
			 p = inverse(M)*p' -  inverse(M) * T
            {p,1} =  |inverse(M)   -inverse(M) * T | * {p', 1}
                          |0                1                     |

	*/

	// 当在更大的网格上计算切线向量的时候，它们往往有很大数量的共享顶点，
	// 当法向贴图应用到这些表面时, 将切线向量平均化通常能获得更好更平滑的结果。
	// 这样做有个问题，就是TBN向量可能会不能互相垂直，这意味着TBN矩阵不再是正交矩阵了。
	// 法线贴图可能会稍稍偏移，但这仍然可以改进。
	// 使用叫做格拉姆-施密特正交化过程（Gram-Schmidt process）的数学技巧

    T = normalize(T - dot(T, N) * N);						
    vec3 B = cross(N, T); 
	// cross是N叉T, 按照右手坐标系+右手螺旋 如果不知道y 就按照xyz顺序, z叉x得到y
    
	// 这里我们使用transpose函数，而不是inverse函数。
	//    正交矩阵（每个轴既是单位向量同时相互垂直）的一大属性是
	//    一个正交矩阵的置换矩阵与它的逆矩阵相等

    mat3 TBN = transpose(mat3(T, B, N));   //   mat3 是列主 T是矩阵第一列
    vs_out.TangentLightPos = TBN * lightPos; 
    vs_out.TangentViewPos  = TBN * viewPos;
    vs_out.TangentFragPos  = TBN * vs_out.FragPos;
        
    gl_Position = projection * view * model * vec4(aPos, 1.0);

	// 转换到切向空间的好处是  
	//     所有光照相关向量(ps需要的光源位置 顶点位置 视线位置) 都可以 在顶点着色器中转换到切线空间
	//     (因为lightPos和viewPos不是每个fragment运行都要改变)
	//     对于 fs_in.FragPos，我们可以计算它在顶点着色器中的切线空间位置, 并让片段插值完成它的工作
	//     这使我们不必在片段着色器中进行矩阵乘法
	//     这是一个很好的优化，因为顶点着色器的运行频率远低于片段着色器。这也是为什么这种方法通常是首选方法的原因。
	//     但是转换到世界坐标系中计算, 就一定要在ps中采样法线贴图后, 对采样的法向量, 转换到世界坐标系了
}