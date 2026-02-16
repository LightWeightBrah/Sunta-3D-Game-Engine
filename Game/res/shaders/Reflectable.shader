#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out			vec3	FragPos;
out			vec3	Normal;
out			vec2	TexCoord;

uniform		mat4	model;
uniform		mat4	view;
uniform		mat4	projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	FragPos		= vec3(model * vec4(aPos, 1.0f));
	Normal		= mat3(transpose(inverse(model))) * aNormal;
	TexCoord	= aTexCoord;
}

#shader fragment
#version 330 core

struct Material
{
	vec3 ambientColor;
	vec3 diffuseColor;
	vec3 specularColor;
	float shininess;
};

struct LightSource
{
	vec3 position;

	vec3 ambientIntensity;
	vec3 diffuseIntensity;
	vec3 specularIntensity;

};

in			vec3			FragPos;
in			vec3			Normal;
in			vec2			TexCoord;

out			vec4			FragColor;

uniform		Material		material;
uniform		LightSource		lightSource;
uniform		vec3			viewerPosition;

uniform		sampler2D		texture_diffuse1;
uniform		sampler2D		texture_specular1;

void main()
{
	// vec4 tex1 = texture(texture_diffuse1, TexCoord);
	// vec4 tex2 = texture(texture_specular1, TexCoord);
	// vec4 combined = mix(tex1, tex2, tex2.a);
	// vec3 baseColor = combined.rgb;
	// vec3 baseColor = texture(texture_diffuse1, TexCoord).rgb;

	// ambient light
	vec3 ambient = lightSource.ambientIntensity * material.ambientColor;//* baseColor;

	// diffuse light
	vec3 norm = normalize(Normal);
	vec3 lightDirection = normalize(lightSource.position - FragPos);
	float dotProductAngle = max(dot(norm, lightDirection), 0.0f);
	vec3 diffuse = lightSource.diffuseIntensity * (dotProductAngle * material.diffuseColor);//* baseColor);

	// specular light
	vec3 viewerDirection = normalize(viewerPosition - FragPos);
	vec3 reflectDirection = reflect(-lightDirection, norm);
	float spec = pow(max(dot(viewerDirection, reflectDirection), 0.0f), material.shininess);
	vec3 specular = lightSource.specularIntensity * (spec * material.specularColor);

	vec3 result = ambient + diffuse + specular;

	FragColor = vec4(result, 1.0f);
}