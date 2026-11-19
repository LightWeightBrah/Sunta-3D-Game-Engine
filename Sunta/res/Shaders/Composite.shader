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

vec3 SceneWithGlow(vec2 coordinate)
{
	vec3 color = texture(scene, coordinate).rgb;
	if (hasBloom)
		color += texture(bloom, coordinate).rgb * bloomIntensity;
	return color;
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
			vec2 at   = TexCoord - step;
			sum += vec3(SceneWithGlow(at + split).r, SceneWithGlow(at).g, SceneWithGlow(at - split).b);
		}
		color = sum / float(Samples);
	}
	else
	{
		color = SceneWithGlow(TexCoord);
	}

	vec3 mapped = ToneMapACES(color * exposure * (1.0 + impact * 0.35));

	// A punch of contrast and a darker rim while it lasts
	mapped = mix(mapped, smoothstep(0.0, 1.0, mapped), impact * 0.8);
	vec2 rim = TexCoord - vec2(0.5);
	mapped *= 1.0 - impact * 0.5 * smoothstep(0.15, 0.55, dot(rim, rim) * 2.0);

	FragColor = vec4(mapped, 1.0);
}
