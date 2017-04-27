#include "stdafx.h"
#include "Utils.h"


namespace zeta
{

	Vector2f::Vector2f() : XMFLOAT2(0, 0) 
	{
	}

	Vector2f::Vector2f(float xx, float yy) : XMFLOAT2(xx, yy)
	{
	}

	Vector2f::Vector2f(const float* arr) : XMFLOAT2(arr)
	{
	}

	XMVECTOR Vector2f::XMV() const
	{
		return XMLoadFloat2(this);
	}

	Vector2f& Vector2f::XMV(const XMVECTOR& v)
	{
		XMStoreFloat2(this, v); return *this;
	}

	Vector3f::Vector3f() : XMFLOAT3(0, 0, 0) 
	{
	}

	Vector3f::Vector3f(float xx, float yy, float zz) : XMFLOAT3(xx, yy, zz) 
	{
	}
	
	Vector3f::Vector3f(const float* arr) : XMFLOAT3(arr)
	{
	}

	XMVECTOR Vector3f::XMV() const 
	{
		return XMLoadFloat3(this);
	}

	Vector3f& Vector3f::XMV(const XMVECTOR& v)
	{
		XMStoreFloat3(this, v); return *this;
	}

	Vector3f CrossProduct3(const Vector3f& v1, const Vector3f& v2)
	{
		Vector3f rv;
		rv.XMV(XMVector3Cross(v1.XMV(), v2.XMV()));
		return rv;
	}

	Vector4f Transform(const Vector4f& v, const Matrix& mat)
	{
		return Vector4f().XMV(XMVector4Transform(v.XMV(), mat));
	}

	Vector3f TransformCoord(const Vector3f& v, const Matrix& mat)
	{
		return Vector3f().XMV(XMVector3TransformCoord(v.XMV(), mat));
	}

	Vector3f TransformNormal(const Vector3f& v, const Matrix& mat)
	{
		return Vector3f().XMV(XMVector3TransformNormal(v.XMV(), mat));
	}


	Vector2f TransformCoord(const Vector2f& v, const Matrix& mat)
	{
		return Vector2f().XMV(XMVector2TransformCoord(v.XMV(), mat));
	}

	Vector2f TransformNormal(const Vector2f& v, const Matrix& mat)
	{
		return Vector2f().XMV(XMVector2TransformNormal(v.XMV(), mat));
	}

	Vector2f Normalize(const Vector2f& v)
	{
		return Vector2f().XMV(XMVector2Normalize(v.XMV()));
	}

	Vector3f Normalize(const Vector3f& v)
	{
		return Vector3f().XMV(XMVector3Normalize(v.XMV()));
	}

	Vector4f Normalize(const Vector4f& v)
	{
		return Vector4f().XMV(XMVector4Normalize(v.XMV()));
	}

	Matrix Inverse(const Matrix& mat)
	{
		return XMMatrixInverse(nullptr, mat);
	}

	float Length(const Vector3f& v)
	{
		return ::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	float Length(const Vector2f& v)
	{
		return ::sqrt(v.x * v.x + v.y * v.y);
	}

	std::wstring ToW(const std::string& str, const std::locale& loc /*= std::locale()*/)
	{
		std::vector<wchar_t> buf(str.size());
		std::use_facet<std::ctype<wchar_t>>(loc).widen(str.data(),//ctype<char_type>  
			str.data() + str.size(),
			buf.data());//把char转换为T  
		return std::wstring(buf.data(), buf.size());
	}

	std::string ToA(const std::wstring& str, const std::locale& loc /*= std::locale()*/)
	{
		std::vector<char> buf(str.size());
		std::use_facet<std::ctype<wchar_t>>(loc).narrow(str.data(),
			str.data() + str.size(),
			'?', buf.data());//把T转换为char  
		return std::string(buf.data(), buf.size());
	}

	std::string CombineFileLine(std::string const & file, int line)
	{
		char str[256];
		sprintf_s(str, "%s: %d", file.c_str(), line);
		return std::string(str);
	}

	Vector4f::Vector4f() : XMFLOAT4(0, 0, 0, 0) 
	{
	}

	Vector4f::Vector4f(float xx, float yy, float zz, float ww) : XMFLOAT4(xx, yy, zz, ww) 
	{
	}

	Vector4f::Vector4f(const float* arr) : XMFLOAT4(arr) 
	{
	}

	XMVECTOR Vector4f::XMV() const 
	{
		return XMLoadFloat4(this);
	}

	Vector4f& Vector4f::XMV(const XMVECTOR& v) 
	{
		XMStoreFloat4(this, v); return *this;
	}

	Matrix::Matrix()
	{
		*(XMMATRIX*)this = XMMatrixIdentity();
	}

	Matrix::Matrix(float m00, float m01, float m02, float m03, 
		float m10, float m11, float m12, float m13, 
		float m20, float m21, float m22, float m23, 
		float m30, float m31, float m32, float m33) 
		: XMMATRIX(
		m00, m01, m02, m03, m10, m11, m12, m13,
		m20, m21, m22, m23, m30, m31, m32, m33)
	{

	}

	Matrix::Matrix(const float *arr) : XMMATRIX(arr)
	{

	}

	Matrix::Matrix(const XMMATRIX& rhs)
	{
		this->operator=(rhs);
	}

	Matrix& Matrix::operator=(const XMMATRIX& m)
	{
		*(XMMATRIX*)this = m; return *this;
	}

	float& Matrix::At(int x, int y)
	{
		float* p = (float*)&r[x];
		return *(p + y);
	}

	float Matrix::At(int x, int y) const
	{
		const float* p = (float*)&r[x];
		return *(p + y);
	}

}