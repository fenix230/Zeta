Texture2D	g_tex;

#include "Utils.fx"


float4 BilinearCopyPS(PP_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;

	return g_tex.Sample(LinearSamplerC, tc);
}


technique11 Copy
{
	pass BilinearCopy
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, BilinearCopyPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}