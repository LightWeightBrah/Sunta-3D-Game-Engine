#shader vertex

layout (location = 0) in vec3 aPos;

out vec2 TexCoord;

void main()
{
	TexCoord    = aPos.xy * 0.5 + 0.5;
	gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment

// The last step (see PostProcessing and learnopengl.com: HDR): the scene in floating point colors plus the blurred glow,
// brought back into the 0..1 the screen can show
// Exposure scales the brightness first, then the ACES filmic curve (Krzysztof Narkowicz's fit) maps any brightness
// into 0..1: dark and middle tones stay almost as they are, very bright ones bend smoothly towards white
// instead of being cut off (a fire up close keeps its shape and color)

in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D scene;
uniform sampler2D bloom;
uniform bool      hasBloom;
uniform float     bloomIntensity;
uniform float     exposure;
uniform float     impact;     // 0..1: the impact frame of a heavy blow (see PostProcessing::Kick)

// Pixel art made of 3D (see EnvironmentComponent): the picture is read in blocks of 'pixelSize' screen pixels, its
// colors brought to 'paletteLevels' per channel through a 4x4 Bayer pattern, edges inked from the depth
uniform float     pixelSize;
uniform float     paletteLevels;
uniform float     ditherStrength;
uniform float     outlineStrength;
uniform vec3      outlineColor;
uniform vec4      depthFromProjection; // xy: the projection's [2][2] and [3][2]
uniform sampler2D sceneDepth;

// The painted look and the grade of the colors (see EnvironmentComponent)
uniform int   paintRadius;
uniform float saturation;
uniform float contrast;
uniform vec3  colorTint;
uniform float vignette;

float MetersAt(vec2 coordinate)
{
	float ndc = texture(sceneDepth, coordinate).r * 2.0 - 1.0;
	return depthFromProjection.y / (ndc + depthFromProjection.x);
}

float Bayer4(vec2 cell)
{
	int x = int(mod(cell.x, 4.0));
	int y = int(mod(cell.y, 4.0));
	int index = x + y * 4;
	// the 4x4 ordered dither matrix, 0..15
	int m[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);
	return (float(m[index]) + 0.5) / 16.0;
}

vec3 SceneWithGlow(vec2 coordinate)
{
	vec3 color = texture(scene, coordinate).rgb;
	if (hasBloom)
		color += texture(bloom, coordinate).rgb * bloomIntensity;
	return color;
}

// Kuwahara filter: of the four squares around the point (each 'paintRadius' pixels wide) the one with the least
// change of color gives its mean - flat strokes of paint with sharp edges where the colors change
vec3 Painted(vec2 coordinate, vec2 texel)
{
	vec3  bestMean     = vec3(0.0);
	float bestVariance = 1e20;
	for (int quadrant = 0; quadrant < 4; quadrant++)
	{
		vec2 direction = vec2(quadrant == 1 || quadrant == 3 ? 1.0 : -1.0, quadrant >= 2 ? 1.0 : -1.0);
		vec3 sum = vec3(0.0), sumOfSquares = vec3(0.0);
		float count = 0.0;
		for (int y = 0; y <= 8; y++)
		{
			if (y > paintRadius) break;
			for (int x = 0; x <= 8; x++)
			{
				if (x > paintRadius) break;
				vec3 c = SceneWithGlow(coordinate + direction * vec2(float(x), float(y)) * texel);
				// (in the tone mapped range: a fire's huge values would win every square otherwise)
				c = c / (1.0 + c);
				sum += c; sumOfSquares += c * c; count += 1.0;
			}
		}
		vec3  mean     = sum / count;
		vec3  spread   = abs(sumOfSquares / count - mean * mean);
		float variance = spread.r + spread.g + spread.b;
		if (variance < bestVariance) { bestVariance = variance; bestMean = mean; }
	}
	return bestMean / max(vec3(1.0) - bestMean, vec3(0.001));
}

vec3 ToneMapACES(vec3 color)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

void main()
{
	vec3 color;

	// One pixel of the picture covers pixelSize x pixelSize of the screen: every screen pixel reads its block's middle
	vec2 screenSize = vec2(textureSize(scene, 0));
	vec2 block      = floor(TexCoord * screenSize / max(pixelSize, 1.0));
	vec2 sampleAt   = pixelSize > 1.0 ? (block + 0.5) * max(pixelSize, 1.0) / screenSize : TexCoord;

	if (impact > 0.001)
	{
		// Blurred out from the middle of the screen, red and blue pulled apart towards the edges
		vec2  fromMiddle = TexCoord - vec2(0.5);
		float edge       = dot(fromMiddle, fromMiddle) * 4.0;
		vec2  split      = fromMiddle * impact * 0.025 * (0.3 + edge);
		vec3  sum        = vec3(0.0);
		const int Samples = 8;
		for (int i = 0; i < Samples; i++)
		{
			vec2 step = fromMiddle * impact * 0.06 * float(i) / float(Samples);
			vec2 at   = sampleAt - step;
			sum += vec3(SceneWithGlow(at + split).r, SceneWithGlow(at).g, SceneWithGlow(at - split).b);
		}
		color = sum / float(Samples);
	}
	else
	{
		color = paintRadius > 0 ? Painted(sampleAt, 1.0 / screenSize) : SceneWithGlow(sampleAt);
	}

	vec3 mapped = ToneMapACES(color * exposure * (1.0 + impact * 0.35));

	// A punch of contrast and a darker rim while it lasts
	mapped = mix(mapped, smoothstep(0.0, 1.0, mapped), impact * 0.8);
	vec2 rim = TexCoord - vec2(0.5);
	mapped *= 1.0 - impact * 0.5 * smoothstep(0.15, 0.55, dot(rim, rim) * 2.0);

	// The grade: tint, saturation around the gray of each color, contrast around the middle, darker corners
	mapped *= colorTint;
	float gray = dot(mapped, vec3(0.2126, 0.7152, 0.0722));
	mapped = clamp(mix(vec3(gray), mapped, saturation), 0.0, 1.0);
	mapped = clamp((mapped - 0.5) * contrast + 0.5, 0.0, 1.0);
	vec2 corner = TexCoord - vec2(0.5);
	mapped *= 1.0 - vignette * smoothstep(0.2, 0.75, length(corner) * 1.4);

	// Ink on the edges: where a neighbouring block is clearly farther away, this block is the outline of a thing
	if (outlineStrength > 0.0)
	{
		vec2  step   = max(pixelSize, 1.0) / screenSize;
		float here   = MetersAt(sampleAt);
		float behind = max(max(MetersAt(sampleAt + vec2(step.x, 0.0)), MetersAt(sampleAt - vec2(step.x, 0.0))),
		                   max(MetersAt(sampleAt + vec2(0.0, step.y)), MetersAt(sampleAt - vec2(0.0, step.y))));
		float edge   = smoothstep(0.06, 0.16, (behind - here) / max(here, 0.001));
		mapped = mix(mapped, outlineColor, edge * outlineStrength);
	}

	// Few colors: each channel brought to paletteLevels steps, the step chosen by the dither pattern of its block
	if (paletteLevels > 1.0)
	{
		float steps     = paletteLevels - 1.0;
		float threshold = (Bayer4(block) - 0.5) * ditherStrength;
		mapped = clamp(floor(mapped * steps + 0.5 + threshold) / steps, 0.0, 1.0);
	}

	FragColor = vec4(mapped, 1.0);
}
