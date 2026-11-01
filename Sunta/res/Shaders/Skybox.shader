#shader vertex

// The sky: a cube around the camera, read from a cubemap by the direction of each of its points
// (see learnopengl.com: Cubemaps). The view matrix without its moving part keeps the sky infinitely far away
layout (location = 0) in vec3 aPos;

uniform mat4 viewRotation; // the view matrix without the camera's position
uniform mat4 projection;

out vec3 direction;

void main()
{
	direction = aPos;
	gl_Position = projection * viewRotation * vec4(aPos, 1.0);
}

#shader fragment

uniform samplerCube sky;
uniform float       brightness;

in vec3 direction;

out vec4 FragColor;

void main()
{
	FragColor = vec4(texture(sky, direction).rgb * brightness, 1.0);
}
