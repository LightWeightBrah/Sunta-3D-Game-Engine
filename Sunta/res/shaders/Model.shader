#shader vertex
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4  aWeights;

out vec2 TexCoord;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 200;
uniform mat4 bones[MAX_BONES];
uniform bool hasAnimations;

void main()
{
    TexCoord = aTexCoord;

    if(!hasAnimations)
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        return;
    }

    mat4 boneTransform = bones[aBoneIDs[0]] * aWeights[0];
    boneTransform     += bones[aBoneIDs[1]] * aWeights[1];
    boneTransform     += bones[aBoneIDs[2]] * aWeights[2];
    boneTransform     += bones[aBoneIDs[3]] * aWeights[3];

    vec4 boneSpacePos = boneTransform * vec4(aPos, 1.0);
    gl_Position = projection * view * model * boneSpacePos;
}

#shader fragment
#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{
    vec4 diffuseColor = texture(texture_diffuse1, TexCoord);
    vec4 specularColor = texture(texture_specular1, TexCoord);
    
    if(diffuseColor.a < 0.1)
        discard;

    FragColor = diffuseColor;
}