Texture2D	g_tex;

#include "Utils.fx"

float3 LinearToSRGB(float3 rgb)
{
	const float ALPHA = 0.055f;
	return rgb < 0.0031308f ? 12.92f * rgb : (1 + ALPHA) * pow(rgb, 1 / 2.4f) - ALPHA;
}


float4 SRGBCorrectionPS(PP_VSO ipt) : SV_Target
{
	float4 c = g_tex.Sample(PointSamplerW, ipt.tc);
	float3 rgb = LinearToSRGB(max(c.rgb, 1e-6f));
	return float4(rgb, 1);
}


technique11 SRGBCorrection
{
	pass SRGBCorrection
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, SRGBCorrectionPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthEnalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}