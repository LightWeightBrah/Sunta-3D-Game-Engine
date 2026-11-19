#shader vertex

// The shapes of every Effect Shape (see EffectShapeComponent, EffectSystem), drawn instanced: one grid of
// (around, across) 0..1, bent here into a ring, an arc, a tube or a ball, sized by the shape's age
layout (location = 0) in vec3 aPos;          // x around, y across
layout (location = 1) in vec4 iColumn0;
layout (location = 2) in vec4 iColumn1;
layout (location = 3) in vec4 iColumn2;
layout (location = 4) in vec4 iColumn3;
layout (location = 5) in vec4 iAgeSeed;      // age 0..1, seed 0..1

uniform mat4  viewProjection;
uniform vec3  cameraPosition;
uniform float fogDensity;

uniform int   kind;          // 0 ring, 1 arc, 2 pillar, 3 sphere, 4 box
uniform float startRadius;
uniform float endRadius;
uniform float thickness;
uniform float height;
uniform float arcRadians;
uniform float spinRadians;   // the whole turn over its life
uniform float twistRadians;

out vec2  uv;
out float age;
out float seed;
out float rim;          // 0 facing the camera, 1 seen edge-on (the bright rim of a ball or a tube)
out float visibility;
out vec2  boxEdge;      // a box: how far from its edges (around, up and down), 0 on an edge
out float nearCamera;   // 0 right at the camera, 1 a few meters away: a blast the camera is inside doesn't fill the screen

const float PI = 3.14159265;

void main()
{
	age  = iAgeSeed.x;
	seed = iAgeSeed.y;
	uv   = aPos.xy;

	// Fast at first, then slowing down, like the front of a blast
	float grown  = 1.0 - pow(1.0 - age, 3.0);
	float radius = mix(startRadius, endRadius, grown);
	float spin   = spinRadians * age;

	vec3 local;
	vec3 normal;
	boxEdge = vec2(1.0);
	if (kind == 4)
	{
		// A cube as a square tube closed at both ends: around its four sides (corners on whole quarters), the
		// bottom cap (the first eighth of the way up: from its middle out to the rim), the sides, the top cap
		float around = fract(aPos.x) * 4.0;
		float side   = min(floor(around), 3.0);
		float t      = around - side;
		vec2  square;
		if (side < 0.5)      square = vec2(-1.0 + 2.0 * t, 1.0);
		else if (side < 1.5) square = vec2(1.0, 1.0 - 2.0 * t);
		else if (side < 2.5) square = vec2(1.0 - 2.0 * t, -1.0);
		else                 square = vec2(-1.0, -1.0 + 2.0 * t);
		if (aPos.x >= 1.0) square = vec2(-1.0, 1.0);

		const float Cap = 0.125;
		float y, spread;
		float upDown;
		if (aPos.y < Cap)             { y = -1.0; spread = aPos.y / Cap;         upDown = spread; normal = vec3(0.0, -1.0, 0.0); }
		else if (aPos.y > 1.0 - Cap)  { y =  1.0; spread = (1.0 - aPos.y) / Cap; upDown = spread; normal = vec3(0.0, 1.0, 0.0); }
		else
		{
			y = mix(-1.0, 1.0, (aPos.y - Cap) / (1.0 - 2.0 * Cap));
			spread = 1.0;
			upDown = 1.0 - abs(y);
			normal = abs(square.x) > abs(square.y) ? vec3(sign(square.x), 0.0, 0.0) : vec3(0.0, 0.0, sign(square.y));
		}
		float c = cos(spin), s = sin(spin);
		vec2 turned = vec2(c * square.x - s * square.y, s * square.x + c * square.y) * spread;
		local   = vec3(turned.x, y, turned.y) * radius;
		normal  = vec3(c * normal.x - s * normal.z, normal.y, s * normal.x + c * normal.z);
		boxEdge = vec2(spread < 0.999 ? 1.0 : min(t, 1.0 - t) * 2.0, upDown);
	}
	else if (kind == 2)
	{
		// A tube: around it, up it (twisting), its top rising as it grows
		float angle = (aPos.x - 0.5) * 2.0 * PI + spin + aPos.y * twistRadians;
		float r     = radius * (1.0 - 0.25 * aPos.y);
		local  = vec3(sin(angle) * r, aPos.y * height * (0.35 + 0.65 * grown), cos(angle) * r);
		normal = vec3(sin(angle), 0.0, cos(angle));
	}
	else if (kind == 3)
	{
		float angle    = (aPos.x - 0.5) * 2.0 * PI + spin;
		float latitude = (aPos.y - 0.5) * PI;
		normal = vec3(cos(latitude) * sin(angle), sin(latitude), cos(latitude) * cos(angle));
		local  = normal * radius;
	}
	else
	{
		// A ring or an arc lying in its own level plane (the effect's turn tilts it), centered on its forward
		float angle = (aPos.x - 0.5) * arcRadians + spin;
		float r     = radius * mix(1.0 - thickness, 1.0, aPos.y);
		local  = vec3(sin(angle) * r, 0.0, cos(angle) * r);
		normal = vec3(0.0, 1.0, 0.0);
	}

	mat4 model = mat4(iColumn0, iColumn1, iColumn2, iColumn3);
	vec4 world = model * vec4(local, 1.0);
	gl_Position = viewProjection * world;

	vec3 worldNormal = normalize(mat3(model) * normal);
	vec3 toCamera    = normalize(cameraPosition - world.xyz);
	rim = 1.0 - abs(dot(worldNormal, toCamera));

	float toCamera_ = length(cameraPosition - world.xyz);
	visibility = exp(-pow(fogDensity * toCamera_, 2.0));
	nearCamera = smoothstep(1.5, 6.0, toCamera_);
}

#shader fragment

uniform int   kind;
uniform vec3  coreColor;     // already times the intensity
uniform vec3  edgeColor;
uniform float noiseScale;
uniform float scrollSpeed;
uniform float dissolve;
uniform float arcRadians;
uniform float lifetimeSeconds;

in vec2  uv;
in float age;
in float seed;
in float rim;
in float visibility;
in vec2  boxEdge;
in float nearCamera;

out vec4 FragColor;

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

// Noise that wraps around (x is an angle 0..1: the seam of a whole ring must not show)
float Fbm(vec2 p, float period)
{
	float sum = 0.0;
	float amplitude = 0.5;
	for (int i = 0; i < 4; i++)
	{
		vec2 q = vec2(mod(p.x, period), p.y);
		vec2 i0 = floor(q);
		vec2 f  = fract(q);
		vec2 u  = f * f * (3.0 - 2.0 * f);
		vec2 i1 = vec2(mod(i0.x + 1.0, period), i0.y);
		float n = mix(mix(Hash(i0), Hash(vec2(i1.x, i0.y)), u.x),
		              mix(Hash(vec2(i0.x, i0.y + 1.0)), Hash(vec2(i1.x, i0.y + 1.0)), u.x), u.y);
		sum += n * amplitude;
		p *= 2.0;
		period *= 2.0;
		amplitude *= 0.5;
	}
	return sum;
}

void main()
{
	float seconds = age * lifetimeSeconds;

	// The pattern flowing through it: around the shape (wrapping) and along/across it
	float period = max(floor(noiseScale * 4.0 * max(arcRadians / 6.2831853, 0.15)), 1.0);
	vec2  p = vec2(uv.x * period, uv.y * noiseScale) + vec2(seed * 17.0, -seconds * scrollSpeed + seed * 5.0);
	float n = Fbm(p, period);

	// How it shines across its band: brightest on the outer edge (the front of the wave, the edge of the blade)
	float band;
	if (kind == 2)
		band = (1.0 - smoothstep(0.55, 1.0, uv.y)) * smoothstep(0.0, 0.08, uv.y) * (0.45 + 0.55 * rim);
	else if (kind == 3)
		band = pow(rim, 1.8) * 0.9 + 0.1;
	else if (kind == 4)
	{
		// Burning edges, faint flowing faces
		float edge = 1.0 - smoothstep(0.0, 0.12, min(boxEdge.x, boxEdge.y));
		band = edge * 1.2 + 0.18 + 0.25 * rim;
	}
	else
		band = smoothstep(0.0, 0.85, uv.y) * (1.0 - smoothstep(0.9, 1.0, uv.y));

	// An arc is a blade's sweep: it appears from its start, its head racing ahead, its tail catching up
	float sweep = 1.0;
	if (kind == 1)
	{
		float head = smoothstep(0.0, 0.35, age) * 1.15;
		float tail = smoothstep(0.25, 1.0, age);
		sweep = smoothstep(tail, tail + 0.25, uv.x) * (1.0 - smoothstep(head - 0.12, head, uv.x));
		// Thinner towards its tail end
		band *= mix(0.35, 1.0, smoothstep(tail, head, uv.x));
	}
	else if (kind == 0)
	{
		// A ring that is only a part of a circle fades out at its ends
		if (arcRadians < 6.2)
			sweep = smoothstep(0.0, 0.15, uv.x) * (1.0 - smoothstep(0.85, 1.0, uv.x));
	}

	// Eaten away by the noise as it dies: more and more of it gone, a hot edge where it burns away
	// (a part of it is always gone: a shape of light is ragged from its birth, never a smooth solid band)
	float threshold = dissolve * (0.22 + 0.9 * smoothstep(0.1, 1.0, age));
	float eaten     = smoothstep(threshold, threshold + 0.08, n);
	float burnEdge  = (1.0 - smoothstep(0.0, 0.1, abs(n - threshold - 0.04))) * step(0.01, threshold);

	float strength = band * sweep * eaten * (0.25 + 1.5 * n * n);
	if (strength <= 0.002 && burnEdge * sweep <= 0.002)
		discard;

	// White-hot where it is strongest, its color at the thin parts; a little brighter at its birth
	float hot   = smoothstep(0.7, 1.15, strength);
	vec3  color = mix(edgeColor, coreColor, hot) * strength + edgeColor * burnEdge * sweep * band * 1.5;
	color *= 1.0 + 0.6 * (1.0 - smoothstep(0.0, 0.12, age));

	FragColor = vec4(color * visibility * nearCamera, 1.0);
}
