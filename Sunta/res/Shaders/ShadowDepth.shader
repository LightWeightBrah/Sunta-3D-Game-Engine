#shader vertex

// The depth of everything that casts shadows, seen from the light (see ShadowRenderer)

layout (location = 0) in  vec3  aPos;

layout (location = 3) in ivec4  aBoneIDs;
layout (location = 4) in  vec4  aWeights;

uniform		mat4	lightSpaceMatrix;
uniform		mat4	model;

const int MAX_BONES = 200;
uniform mat4 bones[MAX_BONES];
uniform bool hasAnimations;

void main()
{
	vec4 localPosition = vec4(aPos, 1.0);

	// The shadow moves with the animation: the same skinning as the Lit shader
	if(hasAnimations)
	{
		mat4 boneTransform = bones[aBoneIDs[0]] * aWeights[0];
		boneTransform     += bones[aBoneIDs[1]] * aWeights[1];
		boneTransform     += bones[aBoneIDs[2]] * aWeights[2];
		boneTransform     += bones[aBoneIDs[3]] * aWeights[3];

		localPosition = boneTransform * localPosition;
	}

	gl_Position = lightSpaceMatrix * model * localPosition;
}

#shader fragment

// Only the depth is kept (written by the graphics card), no color
void main()
{
}
