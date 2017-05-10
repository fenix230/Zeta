Texture2D	g_tex;

float2		g_inv_width_height;

#include "Utils.fx"


float3 FxaaPixelShader(float2 pos)
{
	float2 grad_x = ddx(pos);
	float2 grad_y = ddy(pos);

	float2 posM = pos;

	float4 rgbyM = g_tex.Sample(LinearSamplerW, pos.xy);
	float lumaN = g_tex.Sample(LinearSamplerW, pos.xy + float2(+0, -1) * g_inv_width_height).w;
	float lumaW = g_tex.Sample(LinearSamplerW, pos.xy + float2(-1, +0) * g_inv_width_height).w;
	float lumaM = rgbyM.w;
	float lumaE = g_tex.Sample(LinearSamplerW, pos.xy + float2(+1, +0) * g_inv_width_height).w;
	float lumaS = g_tex.Sample(LinearSamplerW, pos.xy + float2(+0, +1) * g_inv_width_height).w;
	float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
	float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
	float range = rangeMax - rangeMin;

	if (range < max(1 / 24.0f, rangeMax / 8))
	{
		return rgbyM.xyz;
	}
	else
	{
		float lumaNW = g_tex.Sample(LinearSamplerW, pos.xy + float2(-1, -1) * g_inv_width_height).w;
		float lumaNE = g_tex.Sample(LinearSamplerW, pos.xy + float2(+1, -1) * g_inv_width_height).w;
		float lumaSW = g_tex.Sample(LinearSamplerW, pos.xy + float2(-1, +1) * g_inv_width_height).w;
		float lumaSE = g_tex.Sample(LinearSamplerW, pos.xy + float2(+1, +1) * g_inv_width_height).w;

		float lumaNS = lumaN + lumaS;
		float lumaWE = lumaW + lumaE;
		float subpixRcpRange = 1 / range;
		float subpixNSWE = lumaNS + lumaWE;
		float edgeHorz1 = -2 * lumaM + lumaNS;
		float edgeVert1 = -2 * lumaM + lumaWE;

		float lumaNESE = lumaNE + lumaSE;
		float lumaNWNE = lumaNW + lumaNE;
		float edgeHorz2 = -2 * lumaE + lumaNESE;
		float edgeVert2 = -2 * lumaN + lumaNWNE;

		float lumaNWSW = lumaNW + lumaSW;
		float lumaSWSE = lumaSW + lumaSE;
		float edgeHorz4 = abs(edgeHorz1) * 2 + abs(edgeHorz2);
		float edgeVert4 = abs(edgeVert1) * 2 + abs(edgeVert2);
		float edgeHorz3 = -2 * lumaW + lumaNWSW;
		float edgeVert3 = -2 * lumaS + lumaSWSE;
		float edgeHorz = abs(edgeHorz3) + edgeHorz4;
		float edgeVert = abs(edgeVert3) + edgeVert4;

		float subpixNWSWNESE = lumaNWSW + lumaNESE;
		float lengthSign = g_inv_width_height.x;
		bool horzSpan = edgeHorz >= edgeVert;
		float subpixA = subpixNSWE * 2 + subpixNWSWNESE;

		if (horzSpan)
		{
			lengthSign = g_inv_width_height.y;
		}
		else
		{
			lumaN = lumaW;
			lumaS = lumaE;
		}
		float subpixB = subpixA / 12 - lumaM;
		float gradientN = lumaN - lumaM;
		float gradientS = lumaS - lumaM;
		float lumaNN = lumaN + lumaM;
		float lumaSS = lumaS + lumaM;
		bool pairN = abs(gradientN) >= abs(gradientS);
		float gradient = max(abs(gradientN), abs(gradientS));
		if (pairN)
		{
			lengthSign = -lengthSign;
		}
		float subpixC = saturate(abs(subpixB) * subpixRcpRange);

		float2 posB = posM;
		float2 offNP;
		if (horzSpan)
		{
			offNP.x = g_inv_width_height.x;
			offNP.y = 0;
			posB.y += lengthSign * 0.5f;
		}
		else
		{
			offNP.x = 0;
			offNP.y = g_inv_width_height.y;
			posB.x += lengthSign * 0.5f;
		}

		float2 posN = posB - offNP;
		float2 posP = posB + offNP;
		float subpixD = -2 * subpixC + 3;
		float lumaEndN = g_tex.Sample(LinearSamplerW, posN).w;
		float subpixE = subpixC * subpixC;
		float lumaEndP = g_tex.Sample(LinearSamplerW, posP).w;

		if (!pairN)
		{
			lumaNN = lumaSS;
		}
		float gradientScaled = gradient / 4;
		float lumaMM = lumaM - lumaNN * 0.5f;
		float subpixF = subpixD * subpixE;
		bool lumaMLTZero = lumaMM < 0;

		lumaEndN -= lumaNN * 0.5f;
		lumaEndP -= lumaNN * 0.5f;
		bool doneN = abs(lumaEndN) >= gradientScaled;
		bool doneP = abs(lumaEndP) >= gradientScaled;
		if (!doneN)
		{
			posN -= offNP * 1.5f;
		}
		bool doneNP = (!doneN) || (!doneP);
		if (!doneP)
		{
			posP += offNP * 1.5f;
		}

		float step = 2;
		for (int i = 0; (i < 3) && doneNP; ++i)
		{
			step *= i + 1;

			if (!doneN)
			{
				lumaEndN = g_tex.SampleGrad(LinearSamplerW, posN.xy, grad_x, grad_y).w;
				lumaEndN = lumaEndN - lumaNN * 0.5f;
			}
			if (!doneP)
			{
				lumaEndP = g_tex.SampleGrad(LinearSamplerW, posP.xy, grad_x, grad_y).w;
				lumaEndP = lumaEndP - lumaNN * 0.5f;
			}
			doneN = abs(lumaEndN) >= gradientScaled;
			doneP = abs(lumaEndP) >= gradientScaled;
			if (!doneN)
			{
				posN -= offNP * step;
			}
			doneNP = (!doneN) || (!doneP);
			if (!doneP)
			{
				posP += offNP * step;
			}
		}

		float dstN = posM.x - posN.x;
		float dstP = posP.x - posM.x;
		if (!horzSpan)
		{
			dstN = posM.y - posN.y;
			dstP = posP.y - posM.y;
		}

		bool goodSpanN = (lumaEndN < 0) != lumaMLTZero;
		float spanLength = (dstP + dstN);
		bool goodSpanP = (lumaEndP < 0) != lumaMLTZero;
		float spanLengthRcp = 1 / spanLength;

		bool directionN = dstN < dstP;
		float dst = min(dstN, dstP);
		bool goodSpan = directionN ? goodSpanN : goodSpanP;
		float subpixG = subpixF * subpixF;
		float pixelOffset = dst * -spanLengthRcp + 0.5f;
		float subpixH = subpixG * 0.75f;

		float pixelOffsetGood = goodSpan ? pixelOffset : 0;
		float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
		if (horzSpan)
		{
			posM.y += pixelOffsetSubpix * lengthSign;
		}
		else
		{
			posM.x += pixelOffsetSubpix * lengthSign;
		}

		return g_tex.Sample(LinearSamplerW, posM).xyz;
	}
}


float4 CopyRGBLPS(PP_VSO ipt) : SV_Target
{
	const float3 RGB_TO_LUM = float3(0.2126f, 0.7152f, 0.0722f);

	float3 c = g_tex.Sample(PointSamplerC, ipt.tc).rgb;
	float lum = dot(c, RGB_TO_LUM);

	return float4(c, lum);
}


float4 FXAAPS(PP_VSO ipt) : SV_Target
{
	return float4(FxaaPixelShader(ipt.tc), 1.0f);
}


technique11 FXAA
{
	pass CopyRGBL
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, CopyRGBLPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}

	pass FXAA
	{
		SetVertexShader(CompileShader(vs_5_0, PostProcessVS()));
		SetPixelShader(CompileShader(ps_5_0, FXAAPS()));

		SetRasterizerState(BackSolidRS);
		SetDepthStencilState(DepthDisalbedDSS, 0);
		SetBlendState(DisabledBS, float4(0, 0, 0, 0), 0xFFFFFFFF);
	}
}