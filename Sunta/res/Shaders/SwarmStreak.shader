#shader vertex

// The streak of light behind a falling copy of a Swarm: a ribbon from the copy ('iHead') back along its way ('iTail'),
// turned to face the camera, thinning towards its end
layout (location = 0) in vec3 aPos;    // x 0 at the copy .. 1 at the end, y -1..1 across
layout (location = 1) in vec4 iHead;   // xyz the copy, w the width
layout (location = 2) in vec4 iTail;   // xyz the end of the streak, w the heat (0..1)

uniform mat4  viewProjection;
uniform vec3  cameraPosition;
uniform float fogDensity;

out vec2  along;
out float heat;
out float visibility;

void main()
{
	vec3 head = iHead.xyz;
	vec3 tail = iTail.xyz;
	vec3 way  = tail - head;
	vec3 toCamera = cameraPosition - head;
	vec3 across = cross(way, toCamera);
	across = length(across) > 0.0001 ? normalize(across) : vec3(1.0, 0.0, 0.0);

	float width = iHead.w * (1.0 - 0.75 * aPos.x);
	vec3 worldPosition = head + way * aPos.x + across * aPos.y * width;
	gl_Position = viewProjection * vec4(worldPosition, 1.0);

	along      = aPos.xy;
	heat       = iTail.w;
	visibility = exp(-pow(fogDensity * length(toCamera), 2.0));
}

#shader fragment

uniform vec3 glowColor;

in vec2  along;
in float heat;
in float visibility;

out vec4 FragColor;

void main()
{
	// Brightest at the copy and in the middle of the ribbon, white-hot at its head
	float fade   = pow(1.0 - along.x, 2.0) * (1.0 - smoothstep(0.2, 1.0, abs(along.y)));
	vec3  color  = mix(glowColor, vec3(1.0, 0.9, 0.7) * length(glowColor), pow(1.0 - along.x, 6.0) * 0.6);
	FragColor = vec4(color * heat * visibility, fade);
}
