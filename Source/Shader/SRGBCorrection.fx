Texture2D	g_tex;


SamplerState PointSampler
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};



RasterizerState BackSolidRS
{
	FillMode = Solid;
	CullMode = BACK;
};


DepthStencilState DepthEnalbedDSS
{
	DepthEnable = true;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
};


float2 TexCoordFromPos(float4 pos)
{
	float2 tex = pos.xy / 2;
	tex.y *= -1;
	tex += 0.5f;
	return tex;
}


BlendState DisabledBS
{
	AlphaToCoverageEnable = FALSE;
	BlendEnable[0] = FALSE;
};


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


float3 LinearToSRGB(float3 rgb)
{
	const float ALPHA = 0.055f;
	return rgb < 0.0031308f ? 12.92f * rgb : (1 + ALPHA) * pow(rgb, 1 / 2.4f) - ALPHA;
}


float4 SRGBCorrectionPS(PP_VSO ipt) : SV_Target
{
	float4 c = g_tex.Sample(PointSampler, ipt.tc);
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