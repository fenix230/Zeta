float4		g_sample_offset1;
float4		g_sample_offset2;

Texture2D	g_tex;
Texture2D	g_last_lum_tex;

float		g_frame_delta;


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
	float adapted_lum = g_last_lum_tex.Sample(last_lum_sampler, 0.5f.xx).r;
	float current_lum = exp(g_tex.Sample(src_sampler, 0.5f.xx).r);

	return float4(CalcAdaptedLum(adapted_lum, current_lum), 0, 0, 0);
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
}