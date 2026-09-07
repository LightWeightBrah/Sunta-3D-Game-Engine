#shader vertex

layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	// No model matrix here on purpose
	// aPos already comes in as WORLD-SPACE
	// corner positions (GetOBBCorners() already applied the entity's transform)
	// unlike normal meshes which are in local space and need model * pos

	gl_Position = projection * view * vec4(aPos, 1.0);
}

#shader fragment

uniform vec3 lineColor;

out vec4 FragColor;

void main()
{
	FragColor = vec4(lineColor, 1.0f);
}
