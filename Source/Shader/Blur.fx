Texture2D	g_tex;

float2		g_tex_size;
float		g_color_weight[8];
float		g_tc_offset[8];

bool		g_x_dir;

#include "Utils.fx"


struct BLUR_VSO
{
	float4 pos : SV_Position;
	float4 tc0 : TEXCOORD0;
	float4 tc1 : TEXCOORD1;
	float4 tc2 : TEXCOORD2;
	float4 tc3 : TEXCOORD3;
	float2 tc_ori : TEXCOORD4;
};


BLUR_VSO BlurXVS(float4 pos : POSITION)
{
	BLUR_VSO opt;

	opt.pos = pos;

	float2 tex0 = TexCoordFromPos(pos);
	float4 tex[4];
	for (int i = 0; i < 4; ++i)
	{
		tex[i] = tex0.xyxy + float4(g_tc_offset[i * 2 + 0], 0, g_tc_offset[i * 2 + 1], 0);
	}
	opt.tc0 = tex[0];
	opt.tc1 = tex[1];
	opt.tc2 = tex[2];
	opt.tc3 = tex[3];
	opt.tc_ori = tex0;

	return opt;
}


BLUR_VSO BlurYVS(float4 pos : POSITION)
{
	BLUR_VSO opt;

	opt.pos = pos;

	float2 tex0 = TexCoordFromPos(pos);
	float4 tex[4];
	for (int i = 0; i < 4; ++i)
	{
		tex[i] = tex0.xyxy + float4(0, g_tc_offset[i * 2 + 0], 0, g_tc_offset[i * 2 + 1]);
	}
	opt.tc0 = tex[0];
	opt.tc1 = tex[1];
	opt.tc2 = tex[2];
	opt.tc3 = tex[3];
	opt.tc_ori = tex0;

	return opt;
}


float4 CalcBlur(float4 iTex0, float4 iTex1, float4 iTex2, float4 iTex3, float2 offset)
{
	float4 color = float4(0, 0, 0, 1);
	float4 tex[4] = { iTex0, iTex1, iTex2, iTex3 };

	for (int i = 0; i < 4; ++i)
	{
		tex[i] += offset.xyxy;
		color.rgb += g_tex.Sample(LinearSamplerC, tex[i].xy).rgb * g_color_weight[i * 2 + 0];
		color.rgb += g_tex.Sample(LinearSamplerC, tex[i].zw).rgb * g_color_weight[i * 2 + 1];
	}

	return color;
}


float4 BlurXPS(BLUR_VSO ipt) : SV_Target
{
	float4 iTex0 = ipt.tc0;
	float4 iTex1 = ipt.tc1;
	float4 iTex2 = ipt.tc2;
	float4 iTex3 = ipt.tc3;
	float2 iOriTex = ipt.tc_ori;

	float2 offset = float2((floor(iOriTex.x * g_tex_size.x) + 0.5f) * g_tex_size.y - iOriTex.x, 0);
	return CalcBlur(iTex0, iTex1, iTex2, iTex3, offset);
}


float4 BlurYPS(BLUR_VSO ipt) : SV_Target
{
	float4 iTex0 = ipt.tc0;
	float4 iTex1 = ipt.tc1;
	float4 iTex2 = ipt.tc2;
	float4 iTex3 = ipt.tc3;
	float2 iOriTex = ipt.tc_ori;

	float2 offset = float2(0, (floor(iOriTex.y * g_tex_size.x) + 0.5f) * g_tex_size.y - iOriTex.y);
	return CalcBlur(iTex0, iTex1, iTex2, iTex3, offset);
}


technique11 Blur
{
	pass BlurX
	{
		SetVertexShader(CompileShader(vs_5_0, BlurXVS()));
		SetPixelShader(CompileShader(ps_5_0, BlurXPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass BlurY
	{
		SetVertexShader(CompileShader(vs_5_0, BlurYVS()));
		SetPixelShader(CompileShader(ps_5_0, BlurYPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}