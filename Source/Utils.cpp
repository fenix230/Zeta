#include "stdafx.h"
#include "Utils.h"
#include <chrono>


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

	float LinearToSRGBF(float linear)
	{
		if (linear < 0.0031308f)
		{
			return 12.92f * linear;
		}
		else
		{
			float const ALPHA = 0.055f;
			return (1 + ALPHA) * pow(linear, 1 / 2.4f) - ALPHA;
		}
	}

	float SRGBToLinearF(float srgb)
	{
		if (srgb < 0.04045f)
		{
			return srgb / 12.92f;
		}
		else
		{
			float const ALPHA = 0.055f;
			return pow((srgb + ALPHA) / (1 + ALPHA), 2.4f);
		}
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


	Timer::Timer()
	{
		this->Restart();
	}

	void Timer::Restart()
	{
		start_time_ = this->CurrentTime();
	}

	double Timer::Elapsed() const
	{
		return this->CurrentTime() - start_time_;
	}

	double Timer::ElapsedMax() const
	{
		return (std::chrono::duration<double>::max)().count();
	}

	double Timer::ElapsedMin() const
	{
		return (std::chrono::duration<double>::min)().count();
	}

	double Timer::CurrentTime() const
	{
		std::chrono::high_resolution_clock::time_point tp = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<double>>(tp.time_since_epoch()).count();
	}

	void SetEffectVarShaderResource(ID3DX11Effect* effect, const char* var_name, ID3D11ShaderResourceView *var)
	{
		effect->GetVariableByName(var_name)->AsShaderResource()->SetResource(var);
	}

	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, float var)
	{
		effect->GetVariableByName(var_name)->AsScalar()->SetFloat(var);
	}

	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, const float* var, uint32_t var_count)
	{
		effect->GetVariableByName(var_name)->AsScalar()->SetFloatArray(var, 0, var_count);
	}

	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, int var)
	{
		effect->GetVariableByName(var_name)->AsScalar()->SetInt(var);
	}

	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, const int* var, uint32_t var_count)
	{
		effect->GetVariableByName(var_name)->AsScalar()->SetIntArray(var, 0, var_count);
	}

	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, bool var)
	{
		effect->GetVariableByName(var_name)->AsScalar()->SetBool(var);
	}

	void SetEffectVarScalar(ID3DX11Effect* effect, const char* var_name, const bool* var, uint32_t var_count)
	{
		effect->GetVariableByName(var_name)->AsScalar()->SetBoolArray(var, 0, var_count);
	}

	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const bool* var)
	{
		effect->GetVariableByName(var_name)->AsVector()->SetBoolVector(var);
	}

	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const bool* var, uint32_t var_count)
	{
		effect->GetVariableByName(var_name)->AsVector()->SetBoolVectorArray(var, 0, var_count);
	}

	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const int* var)
	{
		effect->GetVariableByName(var_name)->AsVector()->SetIntVector(var);
	}

	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const int* var, uint32_t var_count)
	{
		effect->GetVariableByName(var_name)->AsVector()->SetIntVectorArray(var, 0, var_count);
	}

	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const float* var)
	{
		effect->GetVariableByName(var_name)->AsVector()->SetFloatVector(var);
	}

	void SetEffectVarVector(ID3DX11Effect* effect, const char* var_name, const float* var, uint32_t var_count)
	{
		effect->GetVariableByName(var_name)->AsVector()->SetFloatVectorArray(var, 0, var_count);
	}

	void SetEffectVarMatrix(ID3DX11Effect* effect, const char* var_name, const float* var)
	{
		effect->GetVariableByName(var_name)->AsMatrix()->SetMatrix(var);
	}

	void SetEffectVarMatrix(ID3DX11Effect* effect, const char* var_name, const float* var, uint32_t var_count)
	{
		effect->GetVariableByName(var_name)->AsMatrix()->SetMatrixArray(var, 0, var_count);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, ID3D11ShaderResourceView *var)
	{
		SetEffectVarShaderResource(effect, var_name, var);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, bool var)
	{
		SetEffectVarScalar(effect, var_name, var);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, int var)
	{
		SetEffectVarScalar(effect, var_name, var);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, float var)
	{
		SetEffectVarScalar(effect, var_name, var);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Vector2f& var)
	{
		SetEffectVarVector(effect, var_name, (float*)&var);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Vector3f& var)
	{
		SetEffectVarVector(effect, var_name, (float*)&var);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Vector4f& var)
	{
		SetEffectVarVector(effect, var_name, (float*)&var);
	}

	void SetEffectVar(ID3DX11Effect* effect, const char* var_name, const Matrix& var)
	{
		SetEffectVarMatrix(effect, var_name, (float*)&var);
	}

}


void DO_THROW_EXCEPTION(std::exception& e)
{
	throw e;
}

void DO_THROW_HR_EXCEPTION(HRESULT hr, std::exception& e)
{
	throw e;
}
