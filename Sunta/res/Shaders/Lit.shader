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

vec3 CalculateDirectionalLights(DirectionalLight light, vec3 normal, 			  vec3 viewerDirection);
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

	for(int i = 0; i < directionalLightsCount; i++)
		result += CalculateDirectionalLights(directionalLights[i], normalVector, viewerDirection);

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

vec3 CalculateDirectionalLights(DirectionalLight light, vec3 normal, vec3 viewerDirection)
{
	vec3 lightDirection   = normalize(-light.direction);
	float dotProductAngle = max(dot(normal, lightDirection), 0.0f);

	vec3 reflectDirection = normalize(reflect(-lightDirection, normal));
	float spec            = pow(max(dot(viewerDirection, reflectDirection), 0.0f), material.shininess);

	vec3 ambient  = light.color.ambientIntensity                     * material.ambientColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb;
	vec3 diffuse  = light.color.diffuseIntensity  * (dotProductAngle * material.diffuseColor  * vec3(texture(materialDiffuseMap1, TexCoord)).rgb);
	vec3 specular = light.color.specularIntensity * (spec 		     * material.specularColor * vec3(texture(materialSpecularMap1, TexCoord)).rgb);

	return (ambient + diffuse + specular);
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
