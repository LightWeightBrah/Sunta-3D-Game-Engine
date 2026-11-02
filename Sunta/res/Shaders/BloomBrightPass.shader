#shader vertex

layout (location = 0) in vec3 aPos;

out vec2 TexCoord;

// A quad over the whole screen: its corners are already in clip space (-1..1)
void main()
{
	TexCoord    = aPos.xy * 0.5 + 0.5;
	gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment

// Bloom, step 1 (see PostProcessing): only what is brighter than the threshold is kept, the rest goes black
// A soft knee around the threshold, so things just a little brighter glow just a little (no hard edge of the glow)

in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D scene;
uniform float     threshold;

void main()
{
	vec3  color      = texture(scene, TexCoord).rgb;
	float brightness = max(color.r, max(color.g, color.b));

	float knee = threshold * 0.5;
	float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
	soft = soft * soft / (4.0 * knee + 0.0001);

	float contribution = max(soft, brightness - threshold) / max(brightness, 0.0001);
	FragColor = vec4(color * contribution, 1.0);
}
