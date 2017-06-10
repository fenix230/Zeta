

SamplerState PointSamplerC
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};


SamplerState PointSamplerW
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};


SamplerState LinearSamplerC
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};


SamplerState LinearSamplerW
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};


SamplerState AnisoSamplerW
{
	Filter = ANISOTROPIC;
	AddressU = Wrap;
	AddressV = Wrap;
};


RasterizerState BackSolidRS
{
	FillMode = Solid;
	CullMode = BACK;
};


RasterizerState FrontSolidRS
{
	FillMode = Solid;
	CullMode = FRONT;
};


RasterizerState DoubleSolidRS
{
	FillMode = Solid;
	CullMode = NONE;
};


DepthStencilState DepthEnalbedDSS
{
	DepthEnable = true;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
};


DepthStencilState DepthDisalbedDSS
{
	DepthEnable = false;
	DepthWriteMask = ZERO;
};


BlendState DisabledBS
{
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = FALSE;
};


float2 TexCoordFromPos(float4 pos)
{
	float2 tex = pos.xy / 2;
	tex.y *= -1;
	tex += 0.5f;
	return tex;
}


struct PP_VSO
{
	float4 pos : SV_Position;
	float2 tc : TEXCOORD0;
};


PP_VSO PostProcessVS(float4 pos : POSITION)
{
	PP_VSO opt;

	opt.pos = pos;
	opt.tc = TexCoordFromPos(pos);

	return opt;
}


#define MAX_SHININESS 8192.0f


float4 StoreGBufferRT0(float3 normal, float glossiness)
{
	float p = sqrt(-normal.z * 8 + 8);
	float2 enc = normal.xy / p + 0.5f;
	float2 enc255 = enc * 255;
	float2 residual = floor(frac(enc255) * 16);
	return float4(float3(floor(enc255), residual.x * 16 + residual.y) / 255, glossiness);
}


float4 StoreGBufferRT1(float3 albedo, float metalness)
{
	return float4(albedo, metalness);
}


void StoreGBufferMRT(float3 normal, float glossiness, float3 albedo, float metalness,
	out float4 mrt_0, out float4 mrt_1)
{
	mrt_0 = StoreGBufferRT0(normal, glossiness);
	mrt_1 = StoreGBufferRT1(albedo, metalness);
}


float3 GetNormal(float4 mrt0)
{
	float nz = floor(mrt0.z * 255) / 16;
	mrt0.xy += float2(floor(nz) / 16, frac(nz)) / 255;
	float2 fenc = mrt0.xy * 4 - 2;
	float f = dot(fenc, fenc);
	float g = sqrt(1 - f / 4);
	float3 normal;
	normal.xy = fenc * g;
	normal.z = f / 2 - 1;
	return normal;
}


float GetGlossiness(float4 mrt0)
{
	return mrt0.w;
}


float3 DiffuseColor(float3 albedo, float metalness)
{
	return albedo * (1 - metalness);
}


float3 SpecularColor(float3 albedo, float metalness)
{
	return lerp(0.04, albedo, metalness);
}


float3 GetDiffuse(float4 mrt1)
{
	return DiffuseColor(mrt1.xyz, mrt1.w);
}


float3 GetSpecular(float4 mrt1)
{
	return SpecularColor(mrt1.xyz, mrt1.w);
}


float Shininess2Glossiness(float shininess)
{
	return log2(shininess) / log2(MAX_SHININESS);
}


float Glossiness2Shininess(float glossiness)
{
	return pow(MAX_SHININESS, glossiness);
}


float3 FresnelTerm(float3 light_vec, float3 halfway_vec, float3 c_spec)
{
	float e_n = saturate(dot(light_vec, halfway_vec));
	return c_spec > 0 ? c_spec + (1 - c_spec) * exp2(-(5.55473f * e_n + 6.98316f) * e_n) : 0;
}


float SpecularNormalizeFactor(float shininess)
{
	return (shininess + 2) / 8;
}


float3 DistributionTerm(float3 halfway_vec, float3 normal, float shininess)
{
	return exp((shininess + 0.775f) * (max(dot(halfway_vec, normal), 0.0f) - 1));
}


float3 SpecularTerm(float3 c_spec, float3 light_vec, float3 halfway_vec, float3 normal, float shininess)
{
	return SpecularNormalizeFactor(shininess) * DistributionTerm(halfway_vec, normal, shininess)
		* FresnelTerm(light_vec, halfway_vec, c_spec);
}