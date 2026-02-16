#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	ourColor = aColor;
	texCoord = aTexCoord;
}

#shader fragment
#version 330 core

out vec4 FragColor;

in vec3 ourColor;
in vec2 texCoord;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform vec3 objectColor;

void main()
{
	vec4 tex1 = texture(texture_diffuse1, texCoord);
	vec4 tex2 = texture(texture_specular1, texCoord);
	vec4 combined = mix(tex1, tex2, tex2.a);

	vec4 tintedColor = combined * vec4(objectColor, 1.0f);

	FragColor = mix(combined, tintedColor, 0.5f);
}