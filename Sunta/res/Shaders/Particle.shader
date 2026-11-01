#shader vertex

// One speck of a Particle Emitter: a square around 'center', turned to face the camera (a billboard)
layout (location = 0) in vec3 aPos;

uniform mat4  viewProjection;
uniform vec3  center;
uniform float size;
uniform vec3  cameraRight;
uniform vec3  cameraUp;

out vec2 cornerOffset;

void main()
{
	cornerOffset = aPos.xy * 2.0; // -1..1 across the square

	vec3 worldPosition = center + (cameraRight * aPos.x + cameraUp * aPos.y) * size;
	gl_Position = viewProjection * vec4(worldPosition, 1.0);
}

#shader fragment

uniform vec3 color;

in vec2 cornerOffset;

out vec4 FragColor;

void main()
{
	// A round, soft spot: brightest in the middle, nothing at the edge of the square
	float distanceFromMiddle = length(cornerOffset);
	float glow = 1.0 - smoothstep(0.0, 1.0, distanceFromMiddle);

	if (glow <= 0.0)
		discard;

	// Added to the picture (additive blending): alpha scales how much light the speck adds
	FragColor = vec4(color, glow * glow);
}
