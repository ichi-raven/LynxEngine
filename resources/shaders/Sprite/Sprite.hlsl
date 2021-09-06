
//attention : (bx, spacey) == set y, binding x (regardless of register type)

cbuffer ModelCB : register(b0, space0)
{
	float4x4 proj;
};

//combined image sampler(set : 1, binding : 0)
Texture2D<float4> tex : register(t0, space1);
SamplerState testSampler : register(s0, space1);

struct VSInput
{
	float3 pos : Position;
	float2 uv0 : TexCoord0;
};

struct VSOutput
{
	float4 pos : SV_Position;
	float2 uv0 : Texcoord0;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output = (VSOutput)0;

	output.pos = mul(proj, float4(input.pos, 1.f));
	output.uv0 = input.uv0;

	return output;
}

float4 PSMain(VSOutput input) : SV_Target0
{
	//return float4(1.f, 0, 0, 1.f);
	return tex.Sample(testSampler, input.uv0);
}