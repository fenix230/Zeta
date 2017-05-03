float4		g_sample_offset1;
float4		g_sample_offset2;

Texture2D	g_tex;
Texture2D	g_last_lum_tex;

float		g_frame_delta;

float2		g_tex_size;
float		g_color_weight[8];
float		g_tc_offset[8];

Texture2D	g_glow_tex_0;
Texture2D	g_glow_tex_1;
Texture2D	g_glow_tex_2;


SamplerState PointSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};


SamplerState LinearSampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};


RasterizerState BackSolidRS
{
	FillMode = Solid;
	CullMode = BACK;
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
	float4 tex[2] = { ipt.tc0, ipt.tc1 };

	float s = 0;
	for (int i = 0; i < 2; ++i)
	{
		s += log(dot(g_tex.Sample(LinearSampler, tex[i].xy).rgb, RGB_TO_LUM) + 0.001f);
		s += log(dot(g_tex.Sample(LinearSampler, tex[i].zw).rgb, RGB_TO_LUM) + 0.001f);
	}

	return float4(s / 4, 0, 0, 0);
}


float4 SumLum4x4IterativePS(SUMLUM_VSO ipt) : SV_Target
{
	float4 tex[2] = { ipt.tc0, ipt.tc1 };

	float s = 0;
	for (int i = 0; i < 2; ++i)
	{
		s += g_tex.Sample(LinearSampler, tex[i].xy).r;
		s += g_tex.Sample(LinearSampler, tex[i].zw).r;
	}

	return float4(s / 4, 0, 0, 0);
}


float CalcAdaptedLum(float adapted_lum, float current_lum)
{
	return adapted_lum + (current_lum - adapted_lum) * (1 - pow(0.98f, 50 * g_frame_delta));
}


float4 AdaptedLumPS(SUMLUM_VSO ipt) : SV_Target
{
	float adapted_lum = g_last_lum_tex.Sample(PointSampler, 0.5f.xx).r;
	float current_lum = exp(g_tex.Sample(LinearSampler, 0.5f.xx).r);

	return float4(CalcAdaptedLum(adapted_lum, current_lum), 0, 0, 0);
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


float4 SqrBrightPS(PP_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;

	float4 clr = g_tex.Sample(LinearSampler, tc);
	return clamp(clr * (clr / 3), 0, 32);
}


float4 BilinearCopyPS(PP_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;

	return g_tex.Sample(LinearSampler, tc);
}


struct BLUR_VSO
{
	float4 tc0 : TEXCOORD0;
	float4 tc1 : TEXCOORD1;
	float4 tc2 : TEXCOORD2;
	float4 tc3 : TEXCOORD3;
	float2 tc_ori : TEXCOORD4;
	float4 pos : SV_Position;
};


BLUR_VSO BlurXVS(float4 pos : POSITION)
{
	BLUR_VSO opt;

	opt.pos = pos;

	float2 Tex0 = TexCoordFromPos(pos);
	float4 tex[4];
	for (int i = 0; i < 4; ++i)
	{
		tex[i] = Tex0.xyxy + float4(g_tc_offset[i * 2 + 0], 0, g_tc_offset[i * 2 + 1], 0);
	}
	opt.tc0 = tex[0];
	opt.tc1 = tex[1];
	opt.tc2 = tex[2];
	opt.tc3 = tex[3];
	opt.tc_ori = Tex0;
}


BLUR_VSO BlurYVS(float4 pos : POSITION)
{
	BLUR_VSO opt;

	opt.pos = pos;

	float2 Tex0 = TexCoordFromPos(pos);
	float4 tex[4];
	for (int i = 0; i < 4; ++i)
	{
		tex[i] = Tex0.xyxy + float4(0, g_tc_offset[i * 2 + 0], 0, g_tc_offset[i * 2 + 1]);
	}
	opt.tc0 = tex[0];
	opt.tc1 = tex[1];
	opt.tc2 = tex[2];
	opt.tc3 = tex[3];
	opt.tc_ori = Tex0;
}


float4 CalcBlur(float4 iTex0, float4 iTex1, float4 iTex2, float4 iTex3, float2 offset)
{
	float4 color = float4(0, 0, 0, 1);
	float4 tex[4] = { iTex0, iTex1, iTex2, iTex3 };

	for (int i = 0; i < 4; ++i)
	{
		tex[i] += offset.xyxy;
		color.rgb += src_tex.Sample(src_sampler, tex[i].xy).rgb * color_weight[i * 2 + 0];
		color.rgb += src_tex.Sample(src_sampler, tex[i].zw).rgb * color_weight[i * 2 + 1];
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

	float2 offset = float2((floor(iOriTex.x * src_tex_size.x) + 0.5f) * src_tex_size.y - iOriTex.x, 0);
	return CalcBlur(iTex0, iTex1, iTex2, iTex3, offset);
}


float4 BlurYPS(BLUR_VSO ipt) : SV_Target
{
	float4 iTex0 = ipt.tc0;
	float4 iTex1 = ipt.tc1;
	float4 iTex2 = ipt.tc2;
	float4 iTex3 = ipt.tc3;
	float2 iOriTex = ipt.tc_ori;

	float2 offset = float2(0, (floor(iOriTex.y * src_tex_size.x) + 0.5f) * src_tex_size.y - iOriTex.y);
	return CalcBlur(iTex0, iTex1, iTex2, iTex3, offset);
}


float4 GlowMergerPS(PP_VSO ipt) : SV_Target
{
	float4 clr0 = glow_tex_0.Sample(bilinear_sampler, ipt.tc);
	float4 clr1 = glow_tex_1.Sample(bilinear_sampler, ipt.tc);
	float4 clr2 = glow_tex_2.Sample(bilinear_sampler, ipt.tc);

	return clr0 * 2.0f + clr1 * 1.15f + clr2 * 0.45f;
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

	pass BilinearCopy
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, BilinearCopyPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

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

	pass GlowMerger
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, GlowMergerPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}