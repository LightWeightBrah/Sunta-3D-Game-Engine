#shader vertex

// The copies of a Foliage, all drawn at once (instancing): each one's place and size, its turn, tint and sway phase.
// The wind bends a copy more the higher a vertex stands (its roots stay in the ground)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 iPlaceSize;
layout (location = 4) in vec4 iTurnTintPhase;

uniform mat4  viewProjection;
uniform float time;
uniform float sway;
uniform float swaySpeed;

out vec3  worldPosition;
out vec3  normal;
out vec2  texCoord;
out float tintFactor;

void main()
{
	float c = cos(iTurnTintPhase.x);
	float s = sin(iTurnTintPhase.x);
	vec3 local = aPos * iPlaceSize.w;
	vec3 turned = vec3(c * local.x + s * local.z, local.y, -s * local.x + c * local.z);

	// The wind: a slow gust over the field plus a quick flutter of each copy, bending by the height squared
	float height = max(aPos.y, 0.0);
	float phase  = iTurnTintPhase.z;
	float gust   = sin(time * swaySpeed * 0.6 + iPlaceSize.x * 0.08 + iPlaceSize.z * 0.05);
	float flutter = sin(time * swaySpeed * 2.3 + phase) * 0.35;
	vec2 bend = vec2(gust + flutter, 0.4 * sin(time * swaySpeed * 1.3 + phase * 1.7)) * sway * height * height * iPlaceSize.w;
	turned.xz += bend;

	worldPosition = iPlaceSize.xyz + turned;
	normal        = vec3(c * aNormal.x + s * aNormal.z, aNormal.y, -s * aNormal.x + c * aNormal.z);
	texCoord      = aTexCoord;
	tintFactor    = iTurnTintPhase.y;
	gl_Position   = viewProjection * vec4(worldPosition, 1.0);
}

#shader fragment

uniform sampler2D diffuseMap;
uniform bool      hasDiffuseMap;
uniform vec3      tint;

uniform vec3  lightDirection;
uniform vec3  lightAmbient;
uniform vec3  lightDiffuse;
uniform vec3  cameraPosition;
uniform vec3  fogColor;
uniform float fogDensity;

in vec3  worldPosition;
in vec3  normal;
in vec2  texCoord;
in float tintFactor;

out vec4 FragColor;

void main()
{
	vec4 picture = hasDiffuseMap ? texture(diffuseMap, texCoord) : vec4(0.4, 0.4, 0.3, 1.0);
	// The cut-out of a card: what is transparent in the picture isn't there
	if (picture.a < 0.45)
		discard;

	vec3 baseColor = picture.rgb * tint * tintFactor;
	// Thin blades: lit from both sides, a little light through them (they never go black)
	float facing = abs(dot(normalize(normal), normalize(-lightDirection)));
	vec3 result = baseColor * (lightAmbient * 1.2 + lightDiffuse * (0.35 + 0.65 * facing));

	float visibility = exp(-pow(fogDensity * length(cameraPosition - worldPosition), 2.0));
	result = mix(fogColor, result, clamp(visibility, 0.0, 1.0));
	FragColor = vec4(result, 1.0);
}
