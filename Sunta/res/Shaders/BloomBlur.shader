#shader vertex

layout (location = 0) in vec3 aPos;

out vec2 TexCoord;

void main()
{
	TexCoord    = aPos.xy * 0.5 + 0.5;
	gl_Position = vec4(aPos.xy, 0.0, 1.0);
}

#shader fragment

// Bloom, step 2 (see PostProcessing and learnopengl.com: Bloom): a Gaussian blur along one direction
// Each pixel becomes a weighted average of itself and 4 neighbours on each side (the weights of a Gaussian curve)
// Blurring horizontally and then vertically gives the same as blurring in a square, for far fewer samples

in  vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D image;
uniform bool      isHorizontal;

const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(image, 0));
	vec2 step      = isHorizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

	vec3 result = texture(image, TexCoord).rgb * weights[0];

	for(int i = 1; i < 5; i++)
	{
		result += texture(image, TexCoord + step * float(i)).rgb * weights[i];
		result += texture(image, TexCoord - step * float(i)).rgb * weights[i];
	}

	FragColor = vec4(result, 1.0);
}
