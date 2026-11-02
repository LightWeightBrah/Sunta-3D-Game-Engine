#shader vertex

// One piece of a Weapon Trail: the ribbon between two records of the blade (its base and tip then and a moment later)
// The square's corner says which point it is: x = base (-) or tip (+), y = the older (-) or the newer (+) record
layout (location = 0) in vec3 aPos;

uniform mat4  viewProjection;
uniform vec3  olderBase;
uniform vec3  olderTip;
uniform vec3  newerBase;
uniform vec3  newerTip;
uniform float olderLife;
uniform float newerLife;

out float alongBlade;
out float life;

void main()
{
	bool isTip   = aPos.x > 0.0;
	bool isNewer = aPos.y > 0.0;

	vec3 worldPosition = isNewer ? (isTip ? newerTip : newerBase) : (isTip ? olderTip : olderBase);
	gl_Position = viewProjection * vec4(worldPosition, 1.0);

	alongBlade = isTip ? 1.0 : 0.0;
	life       = isNewer ? newerLife : olderLife;
}

#shader fragment

uniform vec3 color;

in float alongBlade;
in float life;

out vec4 FragColor;

void main()
{
	// Brightest towards the tip (it moves fastest), fading with age; added to the picture (additive blending)
	float strength = life * life * mix(0.15, 1.0, alongBlade);

	if (strength <= 0.0)
		discard;

	FragColor = vec4(color, strength);
}
