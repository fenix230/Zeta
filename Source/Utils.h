#pragma once
#include <DirectXMath.h>


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
	};
	DEFINE_VECTOR_OPERATORS(Vector2f);


	struct Vector3f : public XMFLOAT3
	{
		Vector3f();
		Vector3f(float xx, float yy, float zz);
		explicit Vector3f(const float* arr);

		XMVECTOR XMV() const;
		Vector3f& XMV(const XMVECTOR& v);
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
	};
	DEFINE_VECTOR_OPERATORS(Vector4f);


	struct Matrix : public XMMATRIX
	{
		Matrix();
		Matrix(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33);
		explicit Matrix(const float *arr);

		Matrix& operator= (const XMMATRIX& m);

		Matrix Inverse() const;
	};


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

	std::wstring ToW(const std::string& str, const std::locale& loc = std::locale());
	std::string ToA(const std::wstring& str, const std::locale& loc = std::locale());

	std::string CombineFileLine(std::string const & file, int line);

	template <typename T>
	inline std::shared_ptr<T> MakeCOMPtr(T* p)
	{
		return p ? std::shared_ptr<T>(p, std::mem_fn(&T::Release)) : std::shared_ptr<T>();
	}
}


#define DO_THROW_MSG(x)	{ throw std::exception(x); }
#define DO_THROW(x)	{ throw std::system_error(std::make_error_code(x), CombineFileLine(__FILE__, __LINE__)); }
#define THROW_FAILED(x)	{ HRESULT _hr = x; if (static_cast<HRESULT>(_hr) < 0) { throw std::runtime_error(CombineFileLine(__FILE__, __LINE__)); } }