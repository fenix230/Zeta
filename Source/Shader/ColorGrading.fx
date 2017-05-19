Texture2D	g_tex;
Texture3D	g_color_grading_tex;
bool		g_srgb_correction;
bool		g_color_grading;

#include "Utils.fx"


float3 LinearToSRGB(float3 rgb)
{
	const float ALPHA = 0.055f;
	return rgb < 0.0031308f ? 12.92f * rgb : (1 + ALPHA) * pow(rgb, 1 / 2.4f) - ALPHA;
}


float3 SRGBCorrection(float3 rgb)
{
	return LinearToSRGB(max(rgb, 1e-6f));
}


float3 Grading(float3 rgb)
{
	rgb = saturate(rgb);

	const float DIM = 16;

	return g_color_grading_tex.Sample(LinearSamplerC, (rgb * (DIM - 1) + 0.5f) / DIM).xyz;
}


float4 ColorGradingPS(PP_VSO ipt) : SV_Target
{
	float3 rgb = g_tex.Sample(PointSamplerW, ipt.tc).rgb;

	if (g_srgb_correction)
	{
		rgb = SRGBCorrection(rgb);
	}
	if (g_color_grading)
	{
		rgb = Grading(rgb);
	}

	return float4(rgb, 1);
}


technique11 ColorGrading
{
	pass ColorGrading
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, ColorGradingPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthEnalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}