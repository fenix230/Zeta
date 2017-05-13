#pragma once
#include <DirectXMath.h>
#include <algorithm>


namespace zeta
{

	using namespace DirectX;

#define DEFINE_VECTOR_OPERATORS(VECTOR)\
	inline VECTOR& operator+= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorAdd(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator-= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorSubtract(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator*= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorMultiply(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator/= (VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorDivide(v1.XMV(), v2.XMV());\
		return v1.XMV(vm);\
	}\
	inline VECTOR& operator*= (VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), s);\
		return v.XMV(vm);\
	}\
	inline VECTOR& operator/= (VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), 1 / s);\
		return v.XMV(vm);\
	}\
	inline VECTOR operator+ (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorAdd(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator- (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorSubtract(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator* (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorMultiply(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator/ (const VECTOR& v1, const VECTOR& v2)\
	{\
		XMVECTOR vm = XMVectorDivide(v1.XMV(), v2.XMV());\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator* (const VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), s);\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator* (float s, const VECTOR& v)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), s);\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator/ (const VECTOR& v, float s)\
	{\
		XMVECTOR vm = XMVectorScale(v.XMV(), 1 / s);\
		return VECTOR().XMV(vm);\
	}\
	inline VECTOR operator+ (const VECTOR& v)\
	{\
		return v;\
	}\
	inline VECTOR operator- (const VECTOR& v)\
	{\
		return v * -1.0f;\
	}


	struct Vector2f : public XMFLOAT2
	{
		Vector2f();
		Vector2f(float xx, float yy);
		explicit Vector2f(const float* arr);

		XMVECTOR XMV() const;
		Vector2f& XMV(const XMVECTOR& v);

		static const int elem_count = 2;
	};
	DEFINE_VECTOR_OPERATORS(Vector2f);


	struct Vector3f : public XMFLOAT3
	{
		Vector3f();
		Vector3f(float xx, float yy, float zz);
		explicit Vector3f(const float* arr);

		XMVECTOR XMV() const;
		Vector3f& XMV(const XMVECTOR& v);

		static const int elem_count = 3;
	};
	DEFINE_VECTOR_OPERATORS(Vector3f);

	Vector3f CrossProduct3(const Vector3f& v1, const Vector3f& v2);


	struct Vector4f : public XMFLOAT4
	{
		Vector4f();
		Vector4f(float xx, float yy, float zz, float ww);
		explicit Vector4f(const float* arr);

		XMVECTOR XMV() const;
		Vector4f& XMV(const XMVECTOR& v);

		static const int elem_count = 4;
	};
	DEFINE_VECTOR_OPERATORS(Vector4f);


	struct Matrix : public XMMATRIX
	{
		Matrix();
		Matrix(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33);
		Matrix(const XMMATRIX& rhs);
		explicit Matrix(const float *arr);

		Matrix& operator= (const XMMATRIX& m);

		float& At(int x, int y);
		float At(int x, int y) const;
	};
	inline Matrix operator* (const Matrix& v1, const Matrix& v2)
	{
		XMMATRIX vm = XMMatrixMultiply(v1, v2);
		return vm;
	}


	Vector4f Transform(const Vector4f& v, const Matrix& mat);
	Vector3f TransformCoord(const Vector3f& v, const Matrix& mat);
	Vector3f TransformNormal(const Vector3f& v, const Matrix& mat);
	Vector2f TransformCoord(const Vector2f& v, const Matrix& mat);
	Vector2f TransformNormal(const Vector2f& v, const Matrix& mat);

	Vector4f Normalize(const Vector4f& v);
	Vector3f Normalize(const Vector3f& v);
	Vector2f Normalize(const Vector2f& v);

	float Length(const Vector2f& v);
	float Length(const Vector3f& v);

	Matrix Inverse(const Matrix& mat);

	std::wstring ToW(const std::string& str, const std::locale& loc = std::locale());
	std::string ToA(const std::wstring& str, const std::locale& loc = std::locale());

	std::string CombineFileLine(std::string const & file, int line);

	float LinearToSRGBF(float linear);
	float SRGBToLinearF(float srgb);

	template<class V>
	V LinearToSRGB(const V& v1)
	{
		V v2;
		auto p1 = stdext::make_checked_array_iterator((float*)&v1, V::elem_count, 0);
		auto p2 = stdext::make_checked_array_iterator((float*)&v2, V::elem_count, 0);
		std::transform(p1, p1 + V::elem_count, p2, LinearToSRGBF);
		return v2;
	}

	template<class V>
	V SRGBToLinear(const V& v1)
	{
		V v2;
		auto p1 = stdext::make_checked_array_iterator((float*)&v1, V::elem_count, 0);
		auto p2 = stdext::make_checked_array_iterator((float*)&v2, V::elem_count, 0);
		std::transform(p1, p1 + V::elem_count, p2, SRGBToLinearF);
		return v2;
	}

	template <typename T>
	inline std::shared_ptr<T> MakeCOMPtr(T* p)
	{
		return p ? std::shared_ptr<T>(p, std::mem_fn(&T::Release)) : std::shared_ptr<T>();
	}


	class Timer
	{
	public:
		Timer();

		void Restart();

		double Elapsed() const;

		double ElapsedMax() const;

		double ElapsedMin() const;

		double CurrentTime() const;

	private:
		double start_time_;
	};


	void SetEffectVarShaderResource(ID3DX11Effect* effect, const char* var_name, ID3D11ShaderResourceView *var);

	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, float var);
	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, const float* var, uint32_t var_count);
	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, int var);
	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, const int* var, uint32_t var_count);
	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, bool var);
	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, const bool* var, uint32_t var_count);

	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const bool* var);
	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const bool* var, uint32_t var_count);
	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const int* var);
	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const int* var, uint32_t var_count);
	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const float* var);
	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const float* var, uint32_t var_count);

	void SetEffectVarMatrix(ID3DX11Effect* effect, const char* var_name, const float* var);
	void SetEffectVarMatrix(ID3DX11Effect* effect, const char* var_name, const float* var, uint32_t var_count);

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, ID3D11ShaderResourceView *var);
	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, bool var);
	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, int var);
	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, float var);
	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Vector2f& var);
	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Vector3f& var);
	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Vector4f& var);
	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Matrix& var);
}


void DO_THROW_EXCEPTION(std::exception& e);
void DO_THROW_HR_EXCEPTION(HRESULT hr, std::exception& e);

#define DO_THROW_MSG(x)	{ DO_THROW_EXCEPTION(std::exception(x)); }
#define DO_THROW(x)	{ DO_THROW_EXCEPTION(std::system_error(std::make_error_code(x), CombineFileLine(__FILE__, __LINE__))); }
#define THROW_FAILED(x)	{ HRESULT _hr = x; if (static_cast<HRESULT>(_hr) < 0) { DO_THROW_HR_EXCEPTION(_hr, std::runtime_error(CombineFileLine(__FILE__, __LINE__))); } }