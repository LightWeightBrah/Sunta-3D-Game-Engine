#shader vertex

// One rectangle of the game's UI: a corner of a unit square (0..1) is stretched over 'rectangle' (in pixels)
layout (location = 0) in vec3 aPos;

uniform mat4 projection;  // pixels of the screen, 0,0 in the top left corner
uniform vec4 rectangle;   // x, y, width, height in pixels
uniform vec4 textureArea; // the part of the texture shown: u, v, width, height

out vec2 textureCoordinates;

void main()
{
	vec2 corner = aPos.xy;

	textureCoordinates = textureArea.xy + corner * textureArea.zw;
	gl_Position = projection * vec4(rectangle.xy + corner * rectangle.zw, 0.0, 1.0);
}

#shader fragment

uniform vec4 color;
uniform bool hasTexture;
uniform sampler2D image;

in vec2 textureCoordinates;

out vec4 FragColor;

void main()
{
	vec4 texel = hasTexture ? texture(image, textureCoordinates) : vec4(1.0);
	FragColor = texel * color;
}
