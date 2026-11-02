#shader vertex

layout (location = 0) in  vec3  aPos;
layout (location = 1) in  vec3  aNormal;
layout (location = 2) in  vec2  aTexCoord;

layout (location = 3) in ivec4  aBoneIDs;
layout (location = 4) in  vec4  aWeights;

out			vec3	FragPos;
out			vec3	Normal;
out			vec2	TexCoord;

uniform		mat4	model;
uniform		mat4	view;
uniform		mat4	projection;

const int MAX_BONES = 200;
uniform mat4 bones[MAX_BONES];
uniform bool hasAnimations;

void main()
{
	vec4 localPosition = vec4(aPos, 1.0);
	vec3 localNormal   = aNormal;

	if(hasAnimations)
    {
		mat4 boneTransform = bones[aBoneIDs[0]] * aWeights[0];
		boneTransform     += bones[aBoneIDs[1]] * aWeights[1];
		boneTransform     += bones[aBoneIDs[2]] * aWeights[2];
		boneTransform     += bones[aBoneIDs[3]] * aWeights[3];

		localPosition = boneTransform       * localPosition;
		localNormal   = mat3(boneTransform) * localNormal;
    }

	gl_Position = projection * view * model * localPosition;
	FragPos		= vec3(model * localPosition);
	Normal		= mat3(transpose(inverse(model))) * localNormal;
	TexCoord	= aTexCoord;
}

#shader fragment

struct Material
{
	vec3  ambientColor;
	vec3  diffuseColor;
	vec3  specularColor;
	float shininess;

    bool  useAlphaCutout;

	// A model's own light (glowing cracks, embers, red-hot metal): added after lighting, so it shines in the dark
	bool  hasEmissiveMap;

};

struct LightColor
{
	vec3 ambientIntensity;
	vec3 diffuseIntensity;
	vec3 specularIntensity;
};

struct Attenuation
{
	float constant;
	float linear;
	float quadratic;
};

struct DirectionalLight
{
	vec3 direction;

	LightColor color;
};

struct PointLight
{
	vec3 position;

	LightColor  color;
	Attenuation attenuation;
};

struct SpotLight
{
	vec3 position;
	vec3 spotlightDirection;

	float innerCutOffAngle;
	float outerCutOffAngle;

	LightColor  color;
	Attenuation attenuation;
};


in			vec3				FragPos;
in			vec3				Normal;
in			vec2				TexCoord;

out			vec4				FragColor;

uniform		Material			material;
uniform		vec3				viewerPosition;

#define 	MAX_DIRECTIONAL_LIGHTS 4
#define 	MAX_POINT_LIGHTS 32
#define 	MAX_SPOT_LIGHTS 16

uniform		DirectionalLight	directionalLights[MAX_DIRECTIONAL_LIGHTS];
uniform		PointLight		 	pointLights[MAX_POINT_LIGHTS];
uniform		SpotLight		 	spotLights[MAX_SPOT_LIGHTS];

uniform 	int 				directionalLightsCount; 
uniform 	int 				pointLightsCount; 
uniform 	int 				spotLightsCount; 

//uniform		sampler2D		texture_diffuse1;    // old texture on top of texture and for models
//uniform		sampler2D		texture_specular1;   // old texture on top of texture and for models

uniform		sampler2D			materialDiffuseMap1;
uniform		sampler2D			materialSpecularMap1;
uniform		sampler2D			materialEmissiveMap1;

// How strongly the emissive map shines (0 = off), set per model by its Model component
uniform		float				emissiveStrength;

// Distance fog (exponential squared, see learnopengl.com): the farther, the more of the fog's color. Density 0 = none
uniform		vec3				fogColor;
uniform		float				fogDensity;

// The shadows of the first directional light (see ShadowRenderer and learnopengl.com: Shadow Mapping)
// The shadow map keeps, for every point of a square around the camera, how far from the light the nearest surface is
uniform		bool				hasShadows;
uniform		sampler2D			shadowMap;
uniform		mat4				lightSpaceMatrix;
uniform		float				shadowStrength;		// 0 = no darkening, 1 = only the ambient light is left
uniform		float				shadowNormalOffset;	// meters along the normal before the map is read (no self-shadowing)
uniform		int					shadowSoftness;		// PCF radius in pixels of the map: 1 = 3x3 samples

float CalculateShadowVisibility(vec3 normal);
vec3 CalculateDirectionalLights(DirectionalLight light, vec3 normal, 			  vec3 viewerDirection, float lightVisibility);
vec3 CalculatePointLights(      PointLight       light, vec3 normal, vec3 fragPos, vec3 viewerDirection);
vec3 CalculateSpotLights(       SpotLight        light, vec3 normal, vec3 fragPos, vec3 viewerDirection);

void main()
{
	// vec4 tex1 = texture(texture_diffuse1, TexCoord);
	// vec4 tex2 = texture(texture_specular1, TexCoord);
	// vec4 combined = mix(tex1, tex2, tex2.a);
	// vec3 baseColor = combined.rgb;
	// vec3 baseColor = texture(texture_diffuse1, TexCoord).rgb;

	float textureAlpha = texture(materialDiffuseMap1, TexCoord).a;

	if(material.useAlphaCutout && textureAlpha < 0.1f)
		discard;

	vec3 normalVector 	 = normalize(Normal);
	vec3 viewerDirection = normalize(viewerPosition - FragPos);

	vec3 result = vec3(0.0f);

	// Only the first directional light (the moon, the sun) casts shadows
	float shadowVisibility = CalculateShadowVisibility(normalVector);

	for(int i = 0; i < directionalLightsCount; i++)
		result += CalculateDirectionalLights(directionalLights[i], normalVector, viewerDirection, i == 0 ? shadowVisibility : 1.0f);

	for(int i = 0; i < pointLightsCount; i++)
		result += CalculatePointLights(pointLights[i], normalVector, FragPos, viewerDirection);

	for(int i = 0; i < spotLightsCount; i++)
		result += CalculateSpotLights(spotLights[i], normalVector, FragPos, viewerDirection);

	if(material.hasEmissiveMap)
		result += texture(materialEmissiveMap1, TexCoord).rgb * emissiveStrength;

	float distanceToViewer = length(viewerPosition - FragPos);
	float visibility       = exp(-pow(fogDensity * distanceToViewer, 2.0));
	result = mix(fogColor, result, clamp(visibility, 0.0, 1.0));

	FragColor = vec4(result, 1.0f);

}

// 1 = in the light, less = in a shadow. Each point is moved into the light's view (lightSpaceMatrix) and its depth there
// compared with the map: farther than what the map keeps = something nearer to the light hides it. The neighbouring
// pixels of the map are compared too and averaged, so the shadow's edge is soft (PCF: percentage-closer filtering)
float CalculateShadowVisibility(vec3 normal)
{
	if(!hasShadows || directionalLightsCount == 0)
		return 1.0f;

	vec4 lightSpacePosition = lightSpaceMatrix * vec4(FragPos + normal * shadowNormalOffset, 1.0f);
	vec3 mapCoords = lightSpacePosition.xyz / lightSpacePosition.w * 0.5f + 0.5f;

	// Beyond the far end of the light's view nothing is shadowed
	if(mapCoords.z > 1.0f)
		return 1.0f;

	const float depthBias = 0.0003f;
	vec2 texelSize = 1.0f / vec2(textureSize(shadowMap, 0));

	float shadow  = 0.0f;
	int   samples = 0;

	for(int x = -shadowSoftness; x <= shadowSoftness; x++)
	{
		for(int y = -shadowSoftness; y <= shadowSoftness; y++)
		{
			float nearestDepth = texture(shadowMap, mapCoords.xy + vec2(x, y) * texelSize).r;
			shadow  += (mapCoords.z - depthBias > nearestDepth) ? 1.0f : 0.0f;
			samples += 1;
		}
	}

	shadow /= float(samples);

	// The shadows fade out at the edge of the square around the camera instead of ending on a line
	vec2  fromCenter = abs(mapCoords.xy * 2.0f - 1.0f);
	float edgeFade   = smoothstep(0.8f, 1.0f, max(fromCenter.x, fromCenter.y));
	shadow *= 1.0f - edgeFade;

	return 1.0f - shadow * shadowStrength;
}

// 'lightVisibility' darkens the light's diffuse and specular part in a shadow (the ambient part stays)
vec3 CalculateDirectionalLights(DirectionalLight light, vec3 normal, vec3 viewerDirection, float lightVisibility)
{
	vec3 lightDirection   = normalize(-light.direction);
	float dotProductAngle = max(dot(normal, lightDirection), 0.0f);

	vec3 reflectDirection = normalize(reflect(-lightDirection, normal));
	float spec            = pow(max(dot(viewerDirection, reflectDirection), 0.0f), material.shininess);

	vec3 ambient  = light.color.ambientIntensity                     * material.ambientColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb;
	vec3 diffuse  = light.color.diffuseIntensity  * (dotProductAngle * material.diffuseColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb);
	vec3 specular = light.color.specularIntensity * (spec 		     * material.specularColor * vec3(texture(materialSpecularMap1, TexCoord)).rgb);

	return ambient + (diffuse + specular) * lightVisibility;
}

vec3 CalculatePointLights(PointLight light, vec3 normal, vec3 fragPos, vec3 viewerDirection)
{
	vec3 lightDirection   = normalize(light.position - fragPos);
	float dotProductAngle = max(dot(normal, lightDirection), 0.0f);

	vec3 reflectDirection = reflect(-lightDirection, normal);
	float spec            = pow(max(dot(viewerDirection, reflectDirection), 0.0f), material.shininess);

	// attenuation
	float distance    = length(light.position - fragPos);
	float attenuation = 1.0f / (light.attenuation.constant + light.attenuation.linear * distance + light.attenuation.quadratic * (distance * distance));

	vec3 ambient  = light.color.ambientIntensity                     * material.ambientColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb;
	vec3 diffuse  = light.color.diffuseIntensity  * (dotProductAngle * material.diffuseColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb);
	vec3 specular = light.color.specularIntensity * (spec 		     * material.specularColor * vec3(texture(materialSpecularMap1, TexCoord)).rgb);

	return (ambient + diffuse + specular) * attenuation;
}

vec3 CalculateSpotLights(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewerDirection)
{
	vec3 lightDirection   = normalize(light.position - fragPos);
	float dotProductAngle = max(dot(normal, lightDirection), 0.0f);

	vec3 reflectDirection = reflect(-lightDirection, normal);
	float spec            = pow(max(dot(viewerDirection, reflectDirection), 0.0f), material.shininess);

	// attenuation
	float distance    = length(light.position - fragPos);
	float attenuation = 1.0f / (light.attenuation.constant + light.attenuation.linear * distance + light.attenuation.quadratic * (distance * distance));

	// spotLights
	float theta     = dot(lightDirection, normalize(-light.spotlightDirection));
	float epsilon   = light.innerCutOffAngle - light.outerCutOffAngle;
	float intensity = clamp((theta - light.outerCutOffAngle) / epsilon, 0.0f, 1.0f);

	vec3 ambient  = light.color.ambientIntensity                     * material.ambientColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb;
	vec3 diffuse  = light.color.diffuseIntensity  * (dotProductAngle * material.diffuseColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb);
	vec3 specular = light.color.specularIntensity * (spec 		     * material.specularColor * vec3(texture(materialSpecularMap1, TexCoord)).rgb);

	return (ambient + diffuse + specular) * attenuation * intensity;
}
