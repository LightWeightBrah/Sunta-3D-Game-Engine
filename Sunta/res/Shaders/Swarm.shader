#shader vertex

// The copies of a Swarm, all drawn at once (instancing): each one's place, turn and size in 'iModel', its heat in 'iGlow'
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in mat4 iModel;   // takes the locations 3..6
layout (location = 7) in vec4 iGlow;

uniform mat4 viewProjection;

out vec3 worldPosition;
out vec3 normal;
out vec2 texCoord;
out vec3 glow;

void main()
{
	vec4 world    = iModel * vec4(aPos, 1.0);
	worldPosition = world.xyz;
	normal        = normalize(mat3(iModel) * aNormal);
	texCoord      = aTexCoord;
	glow          = iGlow.rgb;
	gl_Position   = viewProjection * world;
}

#shader fragment

uniform sampler2D diffuseMap;
uniform bool      hasDiffuseMap;

// One light from far away (the scene's first directional light) and the fog of the scene, as the lit models have
uniform vec3  lightDirection;
uniform vec3  lightAmbient;
uniform vec3  lightDiffuse;
uniform vec3  cameraPosition;
uniform vec3  fogColor;
uniform float fogDensity;

in vec3 worldPosition;
in vec3 normal;
in vec2 texCoord;
in vec3 glow;

out vec4 FragColor;

void main()
{
	vec3 baseColor = hasDiffuseMap ? texture(diffuseMap, texCoord).rgb : vec3(0.6);

	// Lit from both sides (a thin blade turned away from the light isn't black)
	float facing = abs(dot(normalize(normal), normalize(-lightDirection)));
	vec3  result = baseColor * (lightAmbient + lightDiffuse * facing);

	// Hot: it glows from inside, strongest where it faces the camera (the edges stay a little darker)
	vec3  toCamera = normalize(cameraPosition - worldPosition);
	float rim      = abs(dot(normalize(normal), toCamera));
	result += glow * (0.55 + 0.45 * rim) + baseColor * glow * 0.5;

	float visibility = exp(-pow(fogDensity * length(cameraPosition - worldPosition), 2.0));
	result = mix(fogColor, result, clamp(visibility, 0.0, 1.0));

	FragColor = vec4(result, 1.0);
}
