#shader vertex

// A ball of clear liquid (see LiquidOrbComponent): a sphere whose skin moves in and out in slow waves
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4  model;
uniform mat4  view;
uniform mat4  projection;

uniform float seconds;
uniform float wobble;
uniform float wobbleSpeed;

out vec3 worldPosition;
out vec3 worldNormal;
out vec3 localDirection;

// A few waves travelling over the sphere in different directions: together they never repeat visibly
float GetWave(vec3 direction, float time)
{
	return sin(direction.x * 3.1 + time * 1.3)
	     + sin(direction.y * 4.3 - time * 1.7 + 1.0)
	     + sin(direction.z * 2.7 + time * 0.9 + 2.0)
	     + sin((direction.x + direction.y) * 5.2 + time * 2.1) * 0.5;
}

void main()
{
	vec3  direction = normalize(aPos);
	float time      = seconds * wobbleSpeed;
	float wave      = GetWave(direction, time) / 3.5;

	// The skin moves along its normal; the normal leans with the slope of the waves (measured a little to each side)
	vec3 moved = aPos * (1.0 + wave * wobble);

	const float step = 0.05;
	vec3 tangent   = normalize(cross(direction, abs(direction.y) < 0.95 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
	vec3 bitangent = cross(direction, tangent);
	float slopeAlongTangent   = GetWave(normalize(direction + tangent * step), time)   - GetWave(normalize(direction - tangent * step), time);
	float slopeAlongBitangent = GetWave(normalize(direction + bitangent * step), time) - GetWave(normalize(direction - bitangent * step), time);
	vec3 leaningNormal = normalize(direction - (tangent * slopeAlongTangent + bitangent * slopeAlongBitangent) * wobble * 2.5);

	vec4 world = model * vec4(moved, 1.0);

	worldPosition  = world.xyz;
	worldNormal    = normalize(mat3(transpose(inverse(model))) * leaningNormal);
	localDirection = direction;

	gl_Position = projection * view * world;
}

#shader fragment

uniform samplerCube sky;
uniform bool        hasSky;
uniform float       skyBrightness;

uniform vec3  cameraPosition;
uniform vec3  tint;
uniform float brightness;
uniform float clearness;
uniform float seconds;

in vec3 worldPosition;
in vec3 worldNormal;
in vec3 localDirection;

out vec4 FragColor;

// How much thinner than glass the bending is: oil and water bend the light a little less
const float RefractionIndex = 1.0 / 1.33;

// The sky seen in a direction, or a plain grey-blue gradient when the scene has none
vec3 GetSky(vec3 direction)
{
	if (hasSky)
		return texture(sky, direction).rgb * skyBrightness;

	return mix(vec3(0.25, 0.27, 0.3), vec3(0.75, 0.8, 0.9), direction.y * 0.5 + 0.5);
}

void main()
{
	vec3 normal = normalize(worldNormal);
	vec3 toEye  = normalize(cameraPosition - worldPosition);

	float facing  = clamp(dot(normal, toEye), 0.0, 1.0);
	// Schlick's approximation: the rim mirrors, the middle is seen through
	float fresnel = 0.04 + 0.96 * pow(1.0 - facing, 4.0);

	// A ball of liquid is a strong lens: the world behind it appears upside down inside it
	vec3 bent     = refract(-toEye, normal, RefractionIndex);
	vec3 through  = GetSky(-bent) * tint;
	vec3 mirrored = GetSky(reflect(-toEye, normal));

	// Slow milky streaks inside, as the liquid turns
	float streaks = sin(localDirection.y * 9.0 + sin(localDirection.x * 5.0 + seconds * 0.7) * 2.0 + seconds * 0.5) * 0.5 + 0.5;
	through = mix(through, vec3(dot(through, vec3(0.333)) + 0.15) * tint, streaks * 0.25);

	vec3 color = mix(through, mirrored, fresnel) * brightness;

	// Two highlights of lights that are always there (a key light above, a softer one opposite) and a bright rim
	vec3 keyLight  = normalize(vec3(0.4, 0.8, 0.45));
	vec3 fillLight = normalize(vec3(-0.6, 0.3, -0.5));
	color += vec3(1.0)  * pow(max(dot(reflect(-keyLight, normal), toEye), 0.0), 90.0) * 4.0;
	color += vec3(0.85) * pow(max(dot(reflect(-fillLight, normal), toEye), 0.0), 40.0) * 0.9;
	color += vec3(0.9, 0.95, 1.0) * pow(1.0 - facing, 3.0) * 0.55;

	// The middle lets a little of what is behind it through, the rim is solid
	float alpha = mix(1.0 - clearness, 1.0, clamp(fresnel * 3.0 + pow(1.0 - facing, 2.0), 0.0, 1.0));

	FragColor = vec4(color, alpha);
}
