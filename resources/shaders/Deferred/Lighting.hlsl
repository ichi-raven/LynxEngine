
const int MAX_LIGHT_NUM = 16;

struct Light
{
    float3 lightDirection;    // ライトの方向
	uint   lightType;		  //ライトのタイプ(0:directional, 1:point)
    float4 lightColor;        // ライトのカラー
	float3 lightPos;		  // ライトの場所(ポイントライトのみ)
	float  lightRange;		  // ライトの影響範囲(ポイントライトのみ)
};

cbuffer LightCB : register(b0, space0)
{
	Light lights[MAX_LIGHT_NUM];
}

cbuffer CameraCB : register(b1, space0)
{
	float3 cameraPos;
}

cbuffer ShadowCB : register(b2, space0)
{
	float4x4 lightViewProj;
	float4x4 lightViewProjBias;
};

//combined image sampler(set : 1, binding : 0)
Texture2D<float4> albedoTex : register(t0, space1);
SamplerState albedoSampler : register(s0, space1);

//combined image sampler(set : 1, binding : 1)
Texture2D<float4> normalTex : register(t1, space1);
SamplerState normalSampler : register(s1, space1);

//combined image sampler(set : 1, binding : 2)
Texture2D<float4> worldPosTex : register(t2, space1);
SamplerState worldPosSampler : register(s2, space1);

//combined image sampler(set : 1, binding : 3)
Texture2D<float4> shadowMap : register(t3, space1);
SamplerState shadowSampler : register(s3, space1);


struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VSOutput VSMain(uint id : SV_VERTEXID)
{
	float x = float(id / 2);
	float y = float(id % 2);
	VSOutput output;
	output.pos = float4(x * 2.f - 1.f, y * 2.f - 1.f, 0, 1.f);
	output.uv = float2(x, y);

	return output;
}

inline float4 lambert(float3 normal, float3 lightDir, float4 lightColor)
{
	return lightColor * max(dot(normal, lightDir) * -1.f, 0);
}

inline float4 phong(float3 camera, float3 pos, float3 lightDir, float3 normal, float4 lightColor)
{
	return lightColor * pow(max(dot(normalize(camera - pos), reflect(lightDir, normal)) * -1.f, 0), 5.f);
}

float4 PSMain(VSOutput input) : SV_Target0
{
	float4 albedo = albedoTex.Sample(albedoSampler, input.uv);
	float4 normal = normalTex.Sample(normalSampler, input.uv);
	float4 worldPos = worldPosTex.Sample(worldPosSampler, input.uv);

	if(normal.w == 0)
		return albedo;

	float4 ambient = float4(0.2f, 0.2f, 0.2f, 0.f);

	normal = (normal * 2.f) - 1.f;
	normal.w = 1.f;

	float4 lightAll = ambient;
	float4 lightDiffuse, lightSpecular;

	for(uint i = 0; i < MAX_LIGHT_NUM; ++i)
	{
		if(lights[i].lightType == 0)
		{
			lightDiffuse = 1 / 3.141593 * lambert(normal.xyz, normalize(lights[i].lightDirection), lights[i].lightColor);
			lightSpecular = 1 / 3.141593 * phong(cameraPos, worldPos.xyz, normalize(lights[i].lightDirection), normal.xyz, lights[i].lightColor);
		}
		else
		{
			float3 lightDir = lights[i].lightPos - worldPos.xyz;
			float distance = length(lightDir);
			float3 normalizedDir = lightDir / distance;
			float affection = max(0.f, 1.f - 1.f / lights[i].lightRange * distance);
			affection *= affection;//square

			lightDiffuse = affection / 3.141593 * lambert(normal.xyz, normalizedDir, lights[i].lightColor);
			lightSpecular = affection / 3.141593 * phong(cameraPos, worldPos.xyz, normalizedDir, normal.xyz, lights[i].lightColor);
		}
		lightAll += (lightDiffuse + lightSpecular);
	}

	// lightAll.x = min(lightAll.x, 1.f);
	// lightAll.y = min(lightAll.y, 1.f);
	// lightAll.z = min(lightAll.z, 1.f);
	// lightAll.w = min(lightAll.w, 1.f);

	float4 outColor = float4((albedo * lightAll).xyz, albedo.w); 

	float4 shadowPos = mul(lightViewProj, worldPos);
	float4 shadowUV = mul(lightViewProjBias, worldPos);
	float z = shadowPos.z /shadowPos.w;
  	float4 fetchUV = shadowUV / shadowUV.w;
	float depthFromLight = shadowMap.Sample(shadowSampler, fetchUV.xy).r + 0.005;

	if( depthFromLight > z && worldPos.w)
	{
		// in shadow
		outColor.rgb *= 0.5f;
	}

	return outColor;
}