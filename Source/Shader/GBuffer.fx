float3		g_albedo_clr;
bool		g_albedo_map_enabled;
Texture2D	g_albedo_tex;

float2		g_metalness_clr;
Texture2D	g_metalness_tex;

float2		g_glossiness_clr;
Texture2D	g_glossiness_tex;

float4x4	g_model_mat;
float4x4	g_view_mat;
float4x4	g_proj_mat;

#include "Utils.fx"


struct GBUFFER_VSO
{
	float4 pos : SV_Position;
	float3 norm : NORMAL;
	float2 tc : TEXCOORD0;
};


GBUFFER_VSO GBufferVS(
	float4 pos : POSITION,
	float3 norm : NORMAL,
	float2 tc : TEXCOORD0
)
{
	GBUFFER_VSO opt;

	opt.pos = pos;
	opt.pos = mul(opt.pos, g_model_mat);
	opt.pos = mul(opt.pos, g_view_mat);
	opt.pos = mul(opt.pos, g_proj_mat);

	opt.norm = norm;
	opt.norm = mul(opt.norm, (float3x3)g_model_mat);
	opt.norm = mul(opt.norm, (float3x3)g_view_mat);

	opt.tc = tc;

	return opt;
}


struct GBUFFER_PSO
{
	float4 mrt_0 : SV_Target0;
	float4 mrt_1 : SV_Target1;
};


GBUFFER_PSO GBufferPS(GBUFFER_VSO ipt)
{
	float3 normal = ipt.norm;

	float3 albedo = g_albedo_clr.rgb;
	if (g_albedo_map_enabled)
	{
		albedo *= g_albedo_tex.Sample(AnisoSamplerW, ipt.tc).rgb;
	}

	float metalness = g_metalness_clr.r;
	if (g_metalness_clr.y > 0.5f)
	{
		metalness *= g_metalness_tex.Sample(AnisoSamplerW, ipt.tc).r;
	}

	float glossiness = g_glossiness_clr.r;
	if (g_glossiness_clr.y > 0.5f)
	{
		glossiness *= g_glossiness_tex.Sample(AnisoSamplerW, ipt.tc).r;
	}

	GBUFFER_PSO opt;
	StoreGBufferMRT(normal, glossiness, albedo, metalness, opt.mrt_0, opt.mrt_1);

	return opt;
}


technique11 GBuffer
{
	pass GBufferGen
	{
		SetVertexShader(CompileShader(vs_5_0, GBufferVS()));
		SetPixelShader(CompileShader(ps_5_0, GBufferPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthEnalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}