#shader vertex

// The specks of every Particle Emitter, all drawn at once (instancing): a square for each, turned to face the camera
// (a billboard) and turned in the picture by its own rotation
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 iCenterSize;          // xyz where it is, w its size
layout (location = 2) in vec4 iColorOpacity;        // its color (light, or the color of smoke) and how solid it is
layout (location = 3) in vec4 iRotationSeedStyle;   // rotation (radians), seed 0..1, style (0 glow, 1 flame, 2 smoke, 3 spark, 4 ring), age 0..1
layout (location = 4) in vec4 iVelocityStretch;     // where it flies, and how many seconds of its flight a spark is long

uniform mat4  viewProjection;
uniform vec3  cameraRight;
uniform vec3  cameraUp;

uniform vec3  cameraPosition;
uniform float fogDensity;

out vec2  cornerOffset;
out vec4  colorOpacity;
out float seed;
out float style;
out float age;
out float visibility;

void main()
{
	cornerOffset = aPos.xy * 2.0; // -1..1 across the square

	float turn = iRotationSeedStyle.x;
	vec2  corner = vec2(cos(turn) * aPos.x - sin(turn) * aPos.y, sin(turn) * aPos.x + cos(turn) * aPos.y);

	vec3 center = iCenterSize.xyz;
	vec3 worldPosition = center + (cameraRight * corner.x + cameraUp * corner.y) * iCenterSize.w;

	// A spark: a thin streak from where it is back along where it came from, turned to face the camera around
	// its own length (as long as its flight of 'stretch' seconds, never shorter than it is wide)
	float styleValue = iRotationSeedStyle.z;
	if (styleValue > 2.5 && styleValue < 3.5)
	{
		vec3  flight   = iVelocityStretch.xyz * iVelocityStretch.w;
		float length_  = max(length(flight), iCenterSize.w);
		vec3  along    = length(flight) > 0.0001 ? normalize(flight) : cameraUp;
		vec3  toCamera = normalize(cameraPosition - center);
		vec3  across   = cross(along, toCamera);
		across = length(across) > 0.0001 ? normalize(across) : cameraRight;
		worldPosition = center - along * (aPos.y + 0.5) * length_ + across * aPos.x * iCenterSize.w * 0.35;
	}

	gl_Position = viewProjection * vec4(worldPosition, 1.0);

	colorOpacity = iColorOpacity;
	seed  = iRotationSeedStyle.y;
	style = iRotationSeedStyle.z;
	age   = iRotationSeedStyle.w;

	// The same distance fog as the lit models: far specks fade away (their light is added, so fading = less light)
	// and specks right at the camera fade out too (a burst around the camera must not cover the screen)
	float toCamera_ = length(cameraPosition - center);
	visibility = exp(-pow(fogDensity * toCamera_, 2.0)) * smoothstep(0.6, 2.5, toCamera_);
}

#shader fragment

uniform bool isDark;   // a dark speck covers what is behind it (smoke, ash), a light one is added to it

in vec2  cornerOffset;
in vec4  colorOpacity;
in float seed;
in float style;
in float age;
in float visibility;

out vec4 FragColor;

// Value noise and its sum over a few sizes (fbm): the ragged edge of a flame or a puff, its own for each seed
float Hash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float Noise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(Hash(i), Hash(i + vec2(1.0, 0.0)), u.x), mix(Hash(i + vec2(0.0, 1.0)), Hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float Fbm(vec2 p)
{
	float sum = 0.0;
	float amplitude = 0.5;
	for (int i = 0; i < 4; i++)
	{
		sum += Noise(p) * amplitude;
		p *= 2.03;
		amplitude *= 0.5;
	}
	return sum;
}

void main()
{
	float distanceFromMiddle = length(cornerOffset);
	vec3  color   = colorOpacity.rgb;
	float opacity = colorOpacity.a;

	// A tongue of fire: a ragged blob (its noise drifts as it ages), its core white-hot, its edge dark red
	if (style > 0.5 && style < 1.5)
	{
		vec2  p     = cornerOffset * 1.6 + vec2(seed * 37.0, seed * 91.0) + vec2(0.0, -age * 1.8);
		float n     = Fbm(p);
		float shape = 1.0 - smoothstep(0.25, 1.0, distanceFromMiddle + (n - 0.5) * 0.9);
		if (shape <= 0.0)
			discard;

		float core = pow(shape, 2.2);
		vec3  fire = color * shape + vec3(1.0, 0.85, 0.55) * core * length(color) * 0.6;
		FragColor  = vec4(fire * visibility, shape);
		return;
	}

	// A spark: hot along its middle line, its head (y = -1 is the front) brighter than its tail; fading with age
	if (style > 2.5 && style < 3.5)
	{
		float acrossFade = 1.0 - smoothstep(0.0, 1.0, abs(cornerOffset.x));
		float alongFade  = mix(1.0, 0.25, (cornerOffset.y + 1.0) * 0.5);
		float fade       = 1.0 - age * age;
		float spark      = acrossFade * acrossFade * alongFade * fade;
		if (spark <= 0.0)
			discard;
		vec3 hot = color + vec3(1.0, 0.95, 0.85) * length(color) * 0.5 * acrossFade * acrossFade * acrossFade;
		FragColor = vec4(hot * spark * visibility, 1.0);
		return;
	}

	// A ring of shock: a thin ragged circle at the edge of the square, thinner and fainter as it swells
	if (style > 3.5)
	{
		float width  = mix(0.22, 0.05, age);
		float n      = Noise(vec2(atan(cornerOffset.y, cornerOffset.x) * 6.0 + seed * 40.0, age * 3.0));
		float ring   = 1.0 - smoothstep(0.0, width, abs(distanceFromMiddle - 0.85 + (n - 0.5) * 0.08));
		float fade   = (1.0 - age) * (1.0 - age);
		ring *= fade * (0.6 + 0.6 * n);
		if (ring <= 0.0)
			discard;
		FragColor = vec4(color * ring * visibility, 1.0);
		return;
	}

	// A puff of smoke: ragged, thicker in the middle
	if (style > 1.5)
	{
		vec2  p     = cornerOffset * 1.3 + vec2(seed * 53.0, seed * 17.0) + vec2(age * 0.6, 0.0);
		float n     = Fbm(p);
		float shape = 1.0 - smoothstep(0.2, 1.0, distanceFromMiddle + (n - 0.5) * 1.1);
		if (shape <= 0.0)
			discard;

		FragColor = vec4(color * (0.75 + 0.5 * n), shape * opacity * visibility);
		return;
	}

	// A round, soft spot: brightest in the middle, nothing at the edge of the square
	float glow = 1.0 - smoothstep(0.0, 1.0, distanceFromMiddle);

	if (glow <= 0.0)
		discard;

	// A dark speck: its own color over the picture, thinner towards its edge and in the distance
	if (isDark)
	{
		FragColor = vec4(color, glow * opacity * visibility);
		return;
	}

	// Added to the picture (additive blending): alpha scales how much light the speck adds
	FragColor = vec4(color * visibility, glow * glow);
}
