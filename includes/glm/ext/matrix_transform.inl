namespace glm
{
	template<typename genType>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR genType identity()
	{
		return detail::init_gentype<genType, detail::genTypeTrait<genType>::GENTYPE>::identity();
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> translate(mat<4, 4, T, Q> const& m, vec<3, T, Q> const& v)
	{
		mat<4, 4, T, Q> Result(m);
		Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> rotate(mat<4, 4, T, Q> const& m, T angle, vec<3, T, Q> const& v)
	{
		T const a = angle;
		T const c = cos(a);
		T const s = sin(a);

		vec<3, T, Q> axis(normalize(v));
		vec<3, T, Q> temp((T(1) - c) * axis);

		mat<4, 4, T, Q> Rotate;
		Rotate[0][0] = c + temp[0] * axis[0];
		Rotate[0][1] = temp[0] * axis[1] + s * axis[2];
		Rotate[0][2] = temp[0] * axis[2] - s * axis[1];

		Rotate[1][0] = temp[1] * axis[0] - s * axis[2];
		Rotate[1][1] = c + temp[1] * axis[1];
		Rotate[1][2] = temp[1] * axis[2] + s * axis[0];

		Rotate[2][0] = temp[2] * axis[0] + s * axis[1];
		Rotate[2][1] = temp[2] * axis[1] - s * axis[0];
		Rotate[2][2] = c + temp[2] * axis[2];

		mat<4, 4, T, Q> Result;
		Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
		Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
		Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
		Result[3] = m[3];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> rotate_slow(mat<4, 4, T, Q> const& m, T angle, vec<3, T, Q> const& v)
	{
		T const a = angle;
		T const c = cos(a);
		T const s = sin(a);
		mat<4, 4, T, Q> Result;

		vec<3, T, Q> axis = normalize(v);

		Result[0][0] = c + (static_cast<T>(1) - c)      * axis.x     * axis.x;
		Result[0][1] = (static_cast<T>(1) - c) * axis.x * axis.y + s * axis.z;
		Result[0][2] = (static_cast<T>(1) - c) * axis.x * axis.z - s * axis.y;
		Result[0][3] = static_cast<T>(0);

		Result[1][0] = (static_cast<T>(1) - c) * axis.y * axis.x - s * axis.z;
		Result[1][1] = c + (static_cast<T>(1) - c) * axis.y * axis.y;
		Result[1][2] = (static_cast<T>(1) - c) * axis.y * axis.z + s * axis.x;
		Result[1][3] = static_cast<T>(0);

		Result[2][0] = (static_cast<T>(1) - c) * axis.z * axis.x + s * axis.y;
		Result[2][1] = (static_cast<T>(1) - c) * axis.z * axis.y - s * axis.x;
		Result[2][2] = c + (static_cast<T>(1) - c) * axis.z * axis.z;
		Result[2][3] = static_cast<T>(0);

		Result[3] = vec<4, T, Q>(0, 0, 0, 1);
		return m * Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> scale(mat<4, 4, T, Q> const& m, vec<3, T, Q> const& v)
	{
		mat<4, 4, T, Q> Result;
		Result[0] = m[0] * v[0];
		Result[1] = m[1] * v[1];
		Result[2] = m[2] * v[2];
		Result[3] = m[3];
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> scale_slow(mat<4, 4, T, Q> const& m, vec<3, T, Q> const& v)
	{
		mat<4, 4, T, Q> Result(T(1));
		Result[0][0] = v.x;
		Result[1][1] = v.y;
		Result[2][2] = v.z;
		return m * Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> lookAtRH(vec<3, T, Q> const& eye, vec<3, T, Q> const& center, vec<3, T, Q> const& up)
	{
		vec<3, T, Q> const f(normalize(center - eye));  // forward 看向的方向 (传入的时候是 postion+Front了)
		vec<3, T, Q> const s(normalize(cross(f, up)));  // right向量
		vec<3, T, Q> const u(cross(s, f));              // 新的up向量
        
        /*
            这里得到是左手坐标系
       u(up,y)  f(forward,z)
            |  /
            | /
            |/____ s(right,x)
            
         */

        // Opengl是列主
        // Result 并不是二维数组, 而是一个struct里面包含了4个vec, 每个vec代表一列
        // Result[0] 调用的是 mat::operator[] 返回内部的4个vec的第0个, 也就是第0列
		mat<4, 4, T, Q> Result(1); // 1是对角线线上的值 也就是默认初始化为单位矩阵
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
        
		Result[0][1] = u.x; // 因为View矩阵是正交矩阵 逆矩阵就是转置, 这里第一列是 [0][0] = s.x [0][1] = u.x [0][2] =-f.x是做了转置
		Result[1][1] = u.y;
		Result[2][1] = u.z;
        
		Result[0][2] =-f.x; // RH右手的话,会把z轴 目标方向 相反了
		Result[1][2] =-f.y;
		Result[2][2] =-f.z;
        
		Result[3][0] =-dot(s, eye); 
		Result[3][1] =-dot(u, eye);  
		Result[3][2] = dot(f, eye); 
        /*
			从世界到视图坐标系位移是eye  但是从视图坐标系到世界, 就不是-eye了

			世界坐标系描述视图坐标轴是 [s, u, f]
			由于这个是正交矩阵, 所以  视图坐标系描述 世界坐标轴是就 转置 
			[ s
			  u
			  f ]

			-eye 这个向量 在 视图坐标系中 的坐标就是 

			[ s
			  u
			  f]  * (-eye)   =   { s*(-eye), u*(-eye), f*(-eye) }

			-----
			在一个坐标系A到另外一个坐标系B, 是位移了向量delta, 反过来就不是位移了向量delta了
			因为这个位移delta向量，是参考坐标系A的; 从坐标系B到坐标系A, 位移向量就要改成参考坐标系B了
		*/
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> lookAtLH(vec<3, T, Q> const& eye, vec<3, T, Q> const& center, vec<3, T, Q> const& up)
	{
		vec<3, T, Q> const f(normalize(center - eye));
		vec<3, T, Q> const s(normalize(cross(up, f)));
		vec<3, T, Q> const u(cross(f, s));

		mat<4, 4, T, Q> Result(1);
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] = f.x;
		Result[1][2] = f.y;
		Result[2][2] = f.z;
		Result[3][0] = -dot(s, eye);
		Result[3][1] = -dot(u, eye);
		Result[3][2] = -dot(f, eye);
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER mat<4, 4, T, Q> lookAt(vec<3, T, Q> const& eye, vec<3, T, Q> const& center, vec<3, T, Q> const& up)
	{
		if(GLM_CONFIG_CLIP_CONTROL & GLM_CLIP_CONTROL_LH_BIT)
			return lookAtLH(eye, center, up);
		else
			return lookAtRH(eye, center, up);
	}
}//namespace glm
