#shader vertex

layout (location = 0) in vec3 aPos;

void main()
{
	gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment

// Pixel art made of 3D, in three passes (see PostProcessing::DrawPixelArt and AIContext/PIXEL_ART_FUNDAMENTALS.md):
//  0. down: every pixel of a low resolution picture (the screen / pixelSize) takes the color of ONE surface of its
//     block (the nearer one if it covers enough of it: clean silhouettes, no blend of a figure and what is behind it),
//     tone mapped and graded, brought to a palette of color ramps in OKLab (few lightness steps, darker steps turned
//     towards a cold shadow hue and lighter ones towards a warm light hue, the middle of a ramp the most saturated).
//     Its alpha keeps how far the surface is (meters)
//  1. ink: on that low picture - lone pixels of a color unlike all four neighbours (that agree) take theirs, the
//     outline is one pixel on the near side of a jump in depth, in a darker, colder step of the pixel's own ramp
//     (not black), lighter on inner edges; the corner pixel of every L of the outline goes ("doubles": lines stay one
//     pixel thick with even steps)
//  2. up: the low picture over the whole screen, each of its pixels a square of pixelSize screen pixels

out vec4 FragColor;

uniform int       pass;
uniform float     pixelSize;

// pass 0
uniform sampler2D scene;
uniform sampler2D bloom;
uniform sampler2D sceneDepth;
uniform bool      hasBloom;
uniform float     bloomIntensity;
uniform float     exposure;
uniform float     impact;
uniform vec4      depthFromProjection;
uniform float     saturation;
uniform float     contrast;
uniform vec3      colorTint;
uniform float     vignette;
uniform float     paletteLevels; // lightness steps of a ramp (0: no palette)
uniform float     hueShift;      // 0..1: how far the darkest and lightest steps turn towards the shadow and light hues
uniform float     shadowHue;     // degrees (OKLab hue): 290 a violet
uniform float     lightHue;      // degrees: 80 a warm yellow

// pass 1 and 2
uniform sampler2D pixels;
uniform float     outlineStrength;
uniform vec3      outlineColor;
uniform float     outlineFog;
uniform bool      cleanLines;
uniform bool      removeOrphans;

const float Tau = 6.28318530718;

// ========== OKLab (Björn Ottosson): a space where equal steps look equal ==========

vec3 ToLinear(vec3 c)   { return pow(max(c, 0.0), vec3(2.2)); }
vec3 FromLinear(vec3 c) { return pow(max(c, 0.0), vec3(1.0 / 2.2)); }

vec3 ToOklab(vec3 c)
{
	c = ToLinear(c);
	float l = 0.4122214708 * c.r + 0.5363325363 * c.g + 0.0514459929 * c.b;
	float m = 0.2119034982 * c.r + 0.6806995451 * c.g + 0.1073969566 * c.b;
	float s = 0.0883024619 * c.r + 0.2817188376 * c.g + 0.6299787005 * c.b;
	l = pow(l, 1.0 / 3.0); m = pow(m, 1.0 / 3.0); s = pow(s, 1.0 / 3.0);
	return vec3(0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
	            1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
	            0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s);
}

vec3 FromOklab(vec3 lab)
{
	float l = lab.x + 0.3963377774 * lab.y + 0.2158037573 * lab.z;
	float m = lab.x - 0.1055613458 * lab.y - 0.0638541728 * lab.z;
	float s = lab.x - 0.0894841775 * lab.y - 1.2914855480 * lab.z;
	l = l * l * l; m = m * m * m; s = s * s * s;
	vec3 c = vec3( 4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
	              -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
	              -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s);
	return clamp(FromLinear(c), 0.0, 1.0);
}

// Turns 'hue' towards 'target' (radians) by at most 'amount'
float TurnHue(float hue, float target, float amount)
{
	float difference = mod(target - hue + Tau * 1.5, Tau) - Tau * 0.5;
	return hue + clamp(difference, -amount, amount);
}

// A color of the palette: its ramp step of lightness, its hue shifted with the step, its chroma highest mid-ramp
vec3 ToPalette(vec3 color)
{
	vec3  lab    = ToOklab(color);
	float steps  = paletteLevels;
	float L      = clamp((floor(lab.x * steps) + 0.5) / steps, 0.0, 1.0);
	float chroma = length(lab.yz);
	float hue    = atan(lab.z, lab.y);
	// -1 the darkest step .. +1 the lightest
	float place  = L * 2.0 - 1.0;
	float target = radians(place < 0.0 ? shadowHue : lightHue);
	hue    = TurnHue(hue, target, hueShift * abs(place) * 1.1);
	// even a grey takes a little of the shadow's or the light's hue (stone and iron are grey here)
	chroma = chroma * mix(0.75, 1.2, 1.0 - abs(place)) + hueShift * 0.035 * abs(place);
	// few chroma steps and hue sectors: a color of the same ramp is the same color on every pixel
	chroma = floor(chroma / 0.02 + 0.5) * 0.02;
	hue    = floor(hue / (Tau / 24.0) + 0.5) * (Tau / 24.0);
	return FromOklab(vec3(L, chroma * cos(hue), chroma * sin(hue)));
}

// A darker, colder step of the pixel's own ramp: the color of its outline
vec3 InkOf(vec3 color, float darker)
{
	vec3  lab    = ToOklab(color);
	float chroma = length(lab.yz) * 0.9 + 0.02;
	float hue    = TurnHue(atan(lab.z, lab.y), radians(shadowHue), 0.6);
	vec3  ink    = FromOklab(vec3(lab.x * darker, chroma * cos(hue), chroma * sin(hue)));
	return mix(ink, outlineColor, 0.35);
}

// ========== Pass 0: down ==========

float MetersAt(vec2 coordinate)
{
	float ndc = texture(sceneDepth, coordinate).r * 2.0 - 1.0;
	return depthFromProjection.y / (ndc + depthFromProjection.x);
}

vec3 ToneMapACES(vec3 color)
{
	return clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
}

vec4 Down()
{
	vec2 screenSize = vec2(textureSize(scene, 0));
	vec2 texel      = 1.0 / screenSize;
	vec2 corner     = floor(gl_FragCoord.xy) * pixelSize;
	vec2 middle     = (corner + pixelSize * 0.5) * texel;

	// 4 x 4 reads over the block, with the depth of each
	vec3  colors[16];
	float meters[16];
	float nearest = 1e20;
	for (int i = 0; i < 16; i++)
	{
		vec2 at   = (corner + (vec2(i % 4, i / 4) + 0.5) * pixelSize * 0.25) * texel;
		colors[i] = texture(scene, at).rgb;
		meters[i] = MetersAt(at);
		nearest   = min(nearest, meters[i]);
	}
	// The near surface if it covers at least a quarter of the block, otherwise what is behind it
	float limit = nearest * 1.04 + 0.05;
	int   nearCount = 0;
	for (int i = 0; i < 16; i++)
		nearCount += meters[i] <= limit ? 1 : 0;
	bool  takeNear = nearCount >= 4;
	vec3  sum = vec3(0.0);
	float distance = 0.0, count = 0.0;
	for (int i = 0; i < 16; i++)
	{
		if ((meters[i] <= limit) == takeNear)
		{
			sum += colors[i]; distance += meters[i]; count += 1.0;
		}
	}
	vec3 color = sum / count;
	if (hasBloom)
		color += texture(bloom, middle).rgb * bloomIntensity;

	// The impact frame of a heavy blow: red and blue pulled apart from the middle
	if (impact > 0.001)
	{
		vec2 fromMiddle = middle - vec2(0.5);
		vec2 split = fromMiddle * impact * 0.03;
		color = mix(color, vec3(texture(scene, middle + split).r, color.g, texture(scene, middle - split).b), impact);
	}

	vec3 mapped = ToneMapACES(color * exposure * (1.0 + impact * 0.35));
	mapped = mix(mapped, smoothstep(0.0, 1.0, mapped), impact * 0.8);

	// The grade (as Composite.shader), the vignette in steps of the big pixels
	mapped *= colorTint;
	float gray = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
	mapped = clamp(mix(vec3(gray), mapped, saturation), 0.0, 1.0);
	mapped = clamp((mapped - 0.5) * contrast + 0.5, 0.0, 1.0);
	vec2 fromCenter = middle - vec2(0.5);
	mapped *= 1.0 - vignette * smoothstep(0.2, 0.75, length(fromCenter) * 1.4);
	mapped *= 1.0 - impact * 0.5 * smoothstep(0.15, 0.55, dot(fromCenter, fromCenter) * 2.0);

	if (paletteLevels > 1.0)
		mapped = ToPalette(mapped);
	return vec4(mapped, distance / count);
}

// ========== Pass 1: ink ==========

vec4 At(ivec2 p)
{
	ivec2 size = textureSize(pixels, 0);
	return texelFetch(pixels, clamp(p, ivec2(0), size - 1), 0);
}

// 2: this pixel is on the near side of a big jump in depth (the outline of a thing), 1: of a small one (an inner
// edge: an arm before the chest), 0: no edge
int EdgeAt(ivec2 p)
{
	float here   = At(p).a;
	float behind = max(max(At(p + ivec2(1, 0)).a, At(p - ivec2(1, 0)).a), max(At(p + ivec2(0, 1)).a, At(p - ivec2(0, 1)).a));
	float jump   = (behind - here) / max(here, 0.001);
	return jump > 0.12 ? 2 : (jump > 0.045 ? 1 : 0);
}

bool IsLine(ivec2 p) { return EdgeAt(p) == 2; }

vec4 Ink()
{
	ivec2 p    = ivec2(gl_FragCoord.xy);
	vec4  here = At(p);
	vec3  color = here.rgb;

	// A lone pixel unlike its four neighbours (which agree with each other) is noise: it takes their color
	// Noise of a texture on one surface: a pixel whose neighbours (three of the four at least) agree on another color
	// takes it - only where all five lie on the same surface (a thin thing before it, a blade or a sword's tip, stays)
	if (removeOrphans)
	{
		vec4 n[4] = vec4[4](At(p + ivec2(1, 0)), At(p - ivec2(1, 0)), At(p + ivec2(0, 1)), At(p - ivec2(0, 1)));
		const float same = 0.004;
		bool oneSurface = true;
		for (int i = 0; i < 4; i++)
			oneSurface = oneSurface && abs(n[i].a - here.a) < here.a * 0.03 + 0.02;
		for (int i = 0; i < 4 && oneSurface; i++)
		{
			int agree = 0;
			for (int j = 0; j < 4; j++)
				agree += distance(n[i].rgb, n[j].rgb) < same ? 1 : 0;
			if (agree >= 3 && distance(n[i].rgb, color) > same)
			{
				color = n[i].rgb;
				break;
			}
		}
	}

	if (outlineStrength > 0.0)
	{
		int edge = EdgeAt(p);
		if (edge == 2 && cleanLines)
		{
			// The corner of an L (a neighbour of the line beside it and one above or below it, not a straight run):
			// half of them go (a checkerboard decides which, so a step keeps one of its two corner pixels)
			bool left = IsLine(p - ivec2(1, 0)), right = IsLine(p + ivec2(1, 0));
			bool down = IsLine(p - ivec2(0, 1)), up    = IsLine(p + ivec2(0, 1));
			bool corner = (left || right) && (up || down) && !(left && right) && !(up && down);
			if (corner && ((p.x + p.y) & 1) == 0)
				edge = 0;
		}
		if (edge > 0)
		{
			float fog  = exp(-pow(outlineFog * here.a, 2.0));
			float ink  = (edge == 2 ? 1.0 : 0.45) * outlineStrength * fog;
			color = mix(color, InkOf(color, edge == 2 ? 0.45 : 0.72), ink);
		}
	}
	return vec4(color, here.a);
}

// ========== Pass 2: up ==========

vec4 Up()
{
	ivec2 size = textureSize(pixels, 0);
	ivec2 p    = clamp(ivec2(gl_FragCoord.xy / pixelSize), ivec2(0), size - 1);
	return vec4(texelFetch(pixels, p, 0).rgb, 1.0);
}

void main()
{
	if (pass == 0)
		FragColor = Down();
	else if (pass == 1)
		FragColor = Ink();
	else
		FragColor = Up();
}
