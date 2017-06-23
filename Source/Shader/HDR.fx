float4		g_sample_offset1;
float4		g_sample_offset2;

Texture2D	g_tex;
Texture2D	g_last_lum_tex;

float		g_frame_delta;

Texture2D	g_glow_tex_0;
Texture2D	g_glow_tex_1;
Texture2D	g_glow_tex_2;

Texture2D	g_src_tex;
Texture2D	g_lum_tex;
Texture2D	g_bloom_tex;

#include "Utils.fx"


struct SUMLUM_VSO
{
	float4 pos : SV_Position;
	float4 tc0 : TEXCOORD0;
	float4 tc1 : TEXCOORD1;
};


SUMLUM_VSO SumLumVS(float4 pos : POSITION)
{
	SUMLUM_VSO opt;
	opt.pos = pos;

	float2 tex = TexCoordFromPos(pos);
	opt.tc0 = tex.xyxy + g_sample_offset1;
	opt.tc1 = tex.xyxy + g_sample_offset2;

	return opt;
}


float4 SumLum4x4LogPS(SUMLUM_VSO ipt) : SV_Target
{
	const float3 RGB_TO_LUM = float3(0.2126f, 0.7152f, 0.0722f);

	float s = 0;

	s += log(dot(g_tex.Sample(LinearSamplerC, ipt.tc0.xy).rgb, RGB_TO_LUM) + 0.001f);
	s += log(dot(g_tex.Sample(LinearSamplerC, ipt.tc0.zw).rgb, RGB_TO_LUM) + 0.001f);

	s += log(dot(g_tex.Sample(LinearSamplerC, ipt.tc1.xy).rgb, RGB_TO_LUM) + 0.001f);
	s += log(dot(g_tex.Sample(LinearSamplerC, ipt.tc1.zw).rgb, RGB_TO_LUM) + 0.001f);

	return float4(s / 4, 0, 0, 0);
}


float4 SumLum4x4IterativePS(SUMLUM_VSO ipt) : SV_Target
{
	float s = 0;

	s += g_tex.Sample(LinearSamplerC, ipt.tc0.xy).r;
	s += g_tex.Sample(LinearSamplerC, ipt.tc0.zw).r;

	s += g_tex.Sample(LinearSamplerC, ipt.tc1.xy).r;
	s += g_tex.Sample(LinearSamplerC, ipt.tc1.zw).r;

	return float4(s / 4, 0, 0, 0);
}


float CalcAdaptedLum(float adapted_lum, float current_lum)
{
	return adapted_lum + (current_lum - adapted_lum) * (1 - pow(0.98f, 50 * g_frame_delta));
}


float4 AdaptedLumPS(SUMLUM_VSO ipt) : SV_Target
{
	float adapted_lum = g_last_lum_tex.Sample(PointSamplerC, 0.5f.xx).r;
	float current_lum = exp(g_tex.Sample(LinearSamplerC, 0.5f.xx).r);

	float lum = CalcAdaptedLum(adapted_lum, current_lum);
	return float4(lum, 0, 0, 0);
}


float4 SqrBrightPS(PP_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;

	float4 clr = g_tex.Sample(LinearSamplerC, tc);
	return clamp(clr * (clr / 3), 0, 32);
}


float4 GlowMergerPS(PP_VSO ipt) : SV_Target
{
	float4 clr0 = g_glow_tex_0.Sample(LinearSamplerC, ipt.tc);
	float4 clr1 = g_glow_tex_1.Sample(LinearSamplerC, ipt.tc);
	float4 clr2 = g_glow_tex_2.Sample(LinearSamplerC, ipt.tc);

	return clr0 * 2.0f + clr1 * 1.15f + clr2 * 0.45f;
}


struct TONEMAPPING_VSO
{
	float3 tc : TEXCOORD0;
	float4 pos : SV_Position;
};


TONEMAPPING_VSO ToneMappingVS(float4 pos : POSITION)
{
	TONEMAPPING_VSO opt;

	opt.pos = pos;
	opt.tc.xy = TexCoordFromPos(pos);
	float adapted_lum = g_lum_tex.SampleLevel(PointSamplerC, 0.5f.xx, 0).r;
	opt.tc.z = max(0.001f, adapted_lum);

	return opt;
}


float EyeAdaption(float lum)
{
	return lerp(0.2f, lum, 0.5f);
}


float3 ACESFilm(float3 x)
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;

	return (x * (A * x + B)) / (x * (C * x + D) + E);
}


float3 ToneMapping(float3 color, float3 blur, float adapted_lum)
{
	const float3 RGB_TO_LUM = float3(0.2126f, 0.7152f, 0.0722f);
	const float3 BLUE_SHIFT = float3(0.4f, 0.4f, 0.7f);

	color += blur * 0.25f;

	float lum = dot(color, RGB_TO_LUM);

	// martin's modified blue shift
	color = lerp(lum * BLUE_SHIFT, color, saturate(16.0f * lum));

	float adapted_lum_dest = 2 / (max(0.1f, 1 + 10 * EyeAdaption(adapted_lum)));

	return ACESFilm(adapted_lum_dest * color);
}


float4 ToneMappingPS(TONEMAPPING_VSO ipt) : SV_Target
{
	float3 ldr_rgb = saturate(ToneMapping(
		g_src_tex.Sample(LinearSamplerC, ipt.tc.xy).rgb,
		g_bloom_tex.Sample(LinearSamplerC, ipt.tc.xy).rgb,
		ipt.tc.z));
	return float4(ldr_rgb, 1);
}


technique11 HDR
{
	pass SumLumLog
	{
		SetVertexShader(CompileShader(vs_5_0, SumLumVS()));
		SetPixelShader(CompileShader(ps_5_0, SumLum4x4LogPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass SumLumIterative
	{
		SetVertexShader(CompileShader(vs_5_0, SumLumVS()));
		SetPixelShader(CompileShader(ps_5_0, SumLum4x4IterativePS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass AdaptedLum
	{
		SetVertexShader(CompileShader(vs_5_0, SumLumVS()));
		SetPixelShader(CompileShader(ps_5_0, AdaptedLumPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass SqrBright
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, SqrBrightPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass GlowMerger
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, GlowMergerPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass ToneMapping
	{
		SetVertexShader(CompileShader(vs_5_0, ToneMappingVS()));
		SetPixelShader(CompileShader(ps_5_0, ToneMappingPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}