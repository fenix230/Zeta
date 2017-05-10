

SamplerState PointSamplerC
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};


SamplerState PointSamplerW
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};


SamplerState LinearSamplerC
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};


SamplerState LinearSamplerW
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};


SamplerState AnisoSamplerW
{
	Filter = ANISOTROPIC;
	AddressU = Wrap;
	AddressV = Wrap;
};


RasterizerState BackSolidRS
{
	FillMode = Solid;
	CullMode = BACK;
};


RasterizerState FrontSolidRS
{
	FillMode = Solid;
	CullMode = FRONT;
};


RasterizerState DoubleSolidRS
{
	FillMode = Solid;
	CullMode = NONE;
};


DepthStencilState DepthEnalbedDSS
{
	DepthEnable = true;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
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