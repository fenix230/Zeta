float4x4	g_model_mat;
float4x4	g_view_mat;
float4x4	g_proj_mat;
float4x4	g_inv_proj_mat;
float4x4	g_inv_mvp_mat;

Texture2D	g_buffer_tex;
Texture2D	g_buffer_1_tex;
Texture2D	g_shading_tex;
Texture2D	g_depth_tex;

TextureCube	g_skybox_tex;

float3		g_light_pos_es;
float3		g_light_dir_es;
float3		g_light_color;
float4		g_light_falloff_range;
float2		g_spot_light_cos_cone;

Texture2D	g_tex;
float4		g_near_q_far;


#define MAX_SHININESS 8192.0f

#include "Utils.fx"


SamplerState SkyBoxSampler
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
	AddressW = Clamp;
};


DepthStencilState CopyDepthDSS
{
	DepthEnable = true;
	DepthWriteMask = ALL;
	DepthFunc = ALWAYS;
};


DepthStencilState SkyBoxDSS
{
	DepthEnable = true;
	DepthWriteMask = ALL;
	DepthFunc = LESS_EQUAL;
};


DepthStencilState LightingDSS
{
	DepthEnable = false;
	DepthWriteMask = ZERO;

	FRONTFACESTENCILFAIL = true;
	FRONTFACESTENCILFUNC = NOT_EQUAL;
	FRONTFACESTENCILPASS = KEEP;

	BACKFACESTENCILFAIL = true;
	BACKFACESTENCILFUNC = NOT_EQUAL;
	BACKFACESTENCILPASS = KEEP;
};


BlendState LightingBS
{
	BlendEnable[0] = TRUE;
	SrcBlend = ONE;
	DestBlend = ONE;
	BlendOp = ADD;
	SrcBlendAlpha = ONE;
	DestBlendAlpha = ONE;
	BlendOpAlpha = ADD;
	RenderTargetWriteMask[0] = 0xF;
};


struct LIGHTING_VSO
{
	float4 pos : SV_Position;
	float2 tc : TEXCOORD0;
	float3 view_dir : VIEWDIR;
};


LIGHTING_VSO LightingVS(float4 pos : POSITION)
{
	LIGHTING_VSO opt;

	opt.pos = pos;
	opt.tc = TexCoordFromPos(pos);
	opt.view_dir = mul(pos, g_inv_proj_mat).xyz;

	return opt;
}


float4 LightingTestPS(LIGHTING_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;
	float3 view_dir = ipt.view_dir;

	float4 mrt_1 = g_buffer_1_tex.Sample(PointSamplerW, tc);

	float3 c_diff = GetDiffuse(mrt_1);

	float4 shading = float4(c_diff, 1);

	return shading;
}


float4 AmbientLightingPS(LIGHTING_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;
	float3 view_dir = ipt.view_dir;

	float4 mrt_0 = g_buffer_tex.Sample(PointSamplerW, tc);
	float4 mrt_1 = g_buffer_1_tex.Sample(PointSamplerW, tc);
	view_dir = normalize(view_dir);
	float3 normal = GetNormal(mrt_0);
	float glossiness = GetGlossiness(mrt_0);
	float shininess = Glossiness2Shininess(glossiness);
	float3 c_diff = GetDiffuse(mrt_1);
	float3 c_spec = GetSpecular(mrt_1);

	float n_dot_l = 0.5f + 0.5f * dot(g_light_dir_es.xyz, normal);
	float4 shading = float4(max(c_diff * n_dot_l, 0) * g_light_color, 1);

	return shading;
}


float4 DirectionLightingPS(LIGHTING_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;
	float3 view_dir = ipt.view_dir;

	float3 shading = 0;

	float4 mrt_0 = g_buffer_tex.Sample(PointSamplerW, tc);
	float4 mrt_1 = g_buffer_1_tex.Sample(PointSamplerW, tc);
	view_dir = normalize(view_dir);
	float3 normal = GetNormal(mrt_0);
	float shininess = Glossiness2Shininess(GetGlossiness(mrt_0));
	float3 c_diff = GetDiffuse(mrt_1);
	float3 c_spec = GetSpecular(mrt_1);

	float3 dir = g_light_dir_es.xyz;
	float n_dot_l = dot(normal, dir);
	if (n_dot_l > 0)
	{
		float3 halfway = normalize(dir - view_dir);
		float3 spec = SpecularTerm(c_spec, dir, halfway, normal, shininess);
		shading = max((c_diff + spec) * n_dot_l, 0) * g_light_color;
	}

	return float4(shading, 1);
}


float AttenuationTerm(float3 light_pos, float3 pos, float3 atten)
{
	float3 v = light_pos - pos;
	float d2 = dot(v, v);
	float d = sqrt(d2);
	return 1 / dot(atten, float3(1, d, d2));
}


float3 CalcShading(float3 light_pos, float3 pos_es, float3 normal, float3 view_dir,
	float3 c_diff, float3 c_spec, float spec_normalize, float shininess, float2 tc,
	float atten, float2 tc_ddx, float2 tc_ddy)
{
	float3 shading = 0;
	float3 dir = light_pos - pos_es;
	float dist = length(dir);
	if (dist < g_light_falloff_range.w)
	{
		dir /= dist;
		float n_dot_l = dot(normal, dir);
		if (n_dot_l > 0)
		{
			float3 halfway = normalize(dir - view_dir);
			float3 spec = spec_normalize * DistributionTerm(halfway, normal, shininess)
				* FresnelTerm(dir, halfway, c_spec);
			shading = max((c_diff + spec) * (n_dot_l * atten), 0) * g_light_color;
		}
	}

	return shading;
}


float SpotLighting(float3 light_pos, float3 light_dir, float2 cos_cone, float3 pos)
{
	// cos_cone is (cos_outer_cone, cos_inner_cone)

	float3 v = normalize(pos - light_pos);
	float cos_direction = dot(v, light_dir);

	return smoothstep(cos_cone.x, cos_cone.y, cos_direction);
}


float3 CalcSpotShading(float3 pos_es, float3 normal, float3 view_dir,
	float3 c_diff, float3 c_spec, float spec_normalize, float shininess, float2 tc, float2 tc_ddx, float2 tc_ddy)
{
	float3 shading = 0;
	float spot = SpotLighting(g_light_pos_es, g_light_dir_es, g_spot_light_cos_cone, pos_es);
	if (spot > 0)
	{
		float atten_term = AttenuationTerm(g_light_pos_es, pos_es, g_light_falloff_range.xyz);
		shading = CalcShading(g_light_pos_es, pos_es, normal, view_dir,
			c_diff, c_spec, spec_normalize, shininess, tc,
			spot * atten_term, tc_ddx, tc_ddy);
	}

	return shading;
}


float4 SpotLightingPS(LIGHTING_VSO ipt) : SV_Target
{
	float2 tc = ipt.tc;
	float3 view_dir = ipt.view_dir;

	float2 tc_ddx = ddx(tc);
	float2 tc_ddy = ddy(tc);

	float4 shading = float4(0, 0, 0, 1);

	float4 mrt_0 = g_buffer_tex.Sample(PointSamplerW, tc);
	float4 mrt_1 = g_buffer_1_tex.Sample(PointSamplerW, tc);
	view_dir = normalize(view_dir);
	float3 pos_es = view_dir * (g_depth_tex.Sample(PointSamplerW, tc).x / view_dir.z);
	float3 normal = GetNormal(mrt_0);
	float shininess = Glossiness2Shininess(GetGlossiness(mrt_0));
	float3 c_diff = GetDiffuse(mrt_1);
	float3 c_spec = GetSpecular(mrt_1);
	float spec_normalize = SpecularNormalizeFactor(shininess);

	shading.rgb += CalcSpotShading(pos_es, normal, view_dir, c_diff, c_spec, spec_normalize, shininess,
		tc, tc_ddx, tc_ddy);

	return shading;
}


struct SKYBOX_VSO
{
	float4 pos : SV_Position;
	float3 tc : TEXCOORD0;
};


SKYBOX_VSO SkyBoxShadingVS(float4 pos : POSITION)
{
	SKYBOX_VSO opt;

	opt.pos = pos;
	opt.tc = mul(pos, g_inv_mvp_mat).xyz;

	return opt;
}


float4 SkyBoxShadingPS(SKYBOX_VSO ipt) : SV_Target
{
	return g_skybox_tex.Sample(SkyBoxSampler, ipt.tc);
}


float NonLinearDepthToLinear(float depth, float near_mul_q, float q)
{
	return near_mul_q / (q - depth);
}


float4 LinearDepthPS(PP_VSO ipt) : SV_Target
{
	float ld = NonLinearDepthToLinear(g_tex.Sample(PointSamplerW, ipt.tc).r,
	g_near_q_far.x, g_near_q_far.y);

	return float4(ld, ld, ld, ld);
}


struct COPY_SHADING_DEPTH_PSO
{
	float4 clr : SV_Target;
	float depth : SV_Depth;
};


COPY_SHADING_DEPTH_PSO CopyShadingDepthPS(PP_VSO ipt)
{
	COPY_SHADING_DEPTH_PSO opt;

	opt.clr = g_shading_tex.Sample(PointSamplerW, ipt.tc);
	opt.depth = g_depth_tex.Sample(PointSamplerW, ipt.tc);

	return opt;
}


technique11 Lighting
{
	pass LinearDepth
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, LinearDepthPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(LightingDSS, 0);
		SetBlendState(LightingBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass AmbientLighting
	{
		SetVertexShader(CompileShader(vs_5_0, LightingVS()));
		SetPixelShader(CompileShader(ps_5_0, AmbientLightingPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(LightingDSS, 0);
		SetBlendState(LightingBS, float4(1, 1, 1, 1), 0xFFFFFFFF);
	}

	pass DirectionLighting
	{
		SetVertexShader(CompileShader(vs_5_0, LightingVS()));
		SetPixelShader(CompileShader(ps_5_0, DirectionLightingPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(LightingDSS, 0);
		SetBlendState(LightingBS, float4(1, 1, 1, 1), 0xFFFFFFFF);
	}

	pass SpotLighting
	{
		SetVertexShader(CompileShader(vs_5_0, LightingVS()));
		SetPixelShader(CompileShader(ps_5_0, SpotLightingPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(LightingDSS, 0);
		SetBlendState(LightingBS, float4(1, 1, 1, 1), 0xFFFFFFFF);
	}

	pass SkyBoxShading
	{
		SetVertexShader(CompileShader(vs_5_0, SkyBoxShadingVS()));
		SetPixelShader(CompileShader(ps_5_0, SkyBoxShadingPS()));

		SetRasterizerState(DoubleSolidRS);
		SetDepthStencilState(SkyBoxDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass CopyShadingDepth
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, CopyShadingDepthPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(CopyDepthDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}