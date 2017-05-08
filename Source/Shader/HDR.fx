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

Texture2D	g_src_tex;
Texture2D	g_lum_tex;
Texture2D	g_bloom_tex;


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

	float s = 0;

	s += log(dot(g_tex.Sample(LinearSampler, ipt.tc0.xy).rgb, RGB_TO_LUM) + 0.001f);
	s += log(dot(g_tex.Sample(LinearSampler, ipt.tc0.zw).rgb, RGB_TO_LUM) + 0.001f);

	s += log(dot(g_tex.Sample(LinearSampler, ipt.tc1.xy).rgb, RGB_TO_LUM) + 0.001f);
	s += log(dot(g_tex.Sample(LinearSampler, ipt.tc1.zw).rgb, RGB_TO_LUM) + 0.001f);

	return float4(s / 4, 0, 0, 0);
}


float4 SumLum4x4IterativePS(SUMLUM_VSO ipt) : SV_Target
{
	float s = 0;

	s += g_tex.Sample(LinearSampler, ipt.tc0.xy).r;
	s += g_tex.Sample(LinearSampler, ipt.tc0.zw).r;

	s += g_tex.Sample(LinearSampler, ipt.tc1.xy).r;
	s += g_tex.Sample(LinearSampler, ipt.tc1.zw).r;

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

	return opt;
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

	return opt;
}


float4 CalcBlur(float4 iTex0, float4 iTex1, float4 iTex2, float4 iTex3, float2 offset)
{
	float4 color = float4(0, 0, 0, 1);
	float4 tex[4] = { iTex0, iTex1, iTex2, iTex3 };

	for (int i = 0; i < 4; ++i)
	{
		tex[i] += offset.xyxy;
		color.rgb += g_tex.Sample(LinearSampler, tex[i].xy).rgb * g_color_weight[i * 2 + 0];
		color.rgb += g_tex.Sample(LinearSampler, tex[i].zw).rgb * g_color_weight[i * 2 + 1];
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


float4 GlowMergerPS(PP_VSO ipt) : SV_Target
{
	float4 clr0 = g_glow_tex_0.Sample(LinearSampler, ipt.tc);
	float4 clr1 = g_glow_tex_1.Sample(LinearSampler, ipt.tc);
	float4 clr2 = g_glow_tex_2.Sample(LinearSampler, ipt.tc);

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
	float adapted_lum = g_lum_tex.SampleLevel(PointSampler, 0.5f.xx, 0).r;
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
		g_src_tex.Sample(LinearSampler, ipt.tc.xy).rgb,
		g_bloom_tex.Sample(LinearSampler, ipt.tc.xy).rgb,
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

	pass ToneMapping
	{
		SetVertexShader(CompileShader(vs_5_0, ToneMappingVS()));
		SetPixelShader(CompileShader(ps_5_0, ToneMappingPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}