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
	vec3 color = texture(scene, TexCoord).rgb;

	if(hasBloom)
		color += texture(bloom, TexCoord).rgb * bloomIntensity;

	FragColor = vec4(ToneMapACES(color * exposure), 1.0);
}
