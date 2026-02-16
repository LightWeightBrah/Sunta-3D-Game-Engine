#include <vector>

#include "Primitives.h"
#include "Mesh.h"
#include "VertexTypes.h"
#include "VertexLayouts.h"
#include "ResourceManager.h"

class TextureItem;

std::unique_ptr<Mesh> Primitives::CreateCube()
{
	float cubeVertices[] = 
	{
		//positions				//normals			//textures
		//front
		 0.5f,  0.5f,  0.5f,	0.0f, 0.0f, 1.0f,	 1.0f, 1.0f,	//top right		
		 0.5f, -0.5f,  0.5f,	0.0f, 0.0f, 1.0f,	 1.0f, 0.0f,	//bottom right	
		-0.5f, -0.5f,  0.5f,	0.0f, 0.0f, 1.0f,	 0.0f, 0.0f,	//bottom left	
		-0.5f,  0.5f,  0.5f,	0.0f, 0.0f, 1.0f,    0.0f, 1.0f,	//top left		

		//back
		 0.5f,  0.5f, -0.5f,	0.0f, 0.0f, -1.0f,	 1.0f, 1.0f,	//top right		
		 0.5f, -0.5f, -0.5f,	0.0f, 0.0f, -1.0f,	 1.0f, 0.0f,	//bottom right	
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f, -1.0f,	 0.0f, 0.0f,	//bottom left	
		-0.5f,  0.5f, -0.5f,	0.0f, 0.0f, -1.0f,   0.0f, 1.0f,	//top left		

		//up
		 0.5f,  0.5f, -0.5f,	0.0f, 1.0f, 0.0f,	 1.0f, 1.0f,	//top right		
		 0.5f,  0.5f,  0.5f,	0.0f, 1.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
		-0.5f,  0.5f,  0.5f,	0.0f, 1.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
		-0.5f,  0.5f, -0.5f,	0.0f, 1.0f, 0.0f,    0.0f, 1.0f,	//top left		

		//bottom
		 0.5f, -0.5f, -0.5f,	0.0f, -1.0f, 0.0f,	 1.0f, 1.0f,	//top right		
		 0.5f, -0.5f,  0.5f,	0.0f, -1.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
		-0.5f, -0.5f,  0.5f,	0.0f, -1.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
		-0.5f, -0.5f, -0.5f,	0.0f, -1.0f, 0.0f,   0.0f, 1.0f,	//top left

		//right
		 0.5f,  0.5f, -0.5f,	1.0f, 0.0f, 0.0f,	 1.0f, 1.0f,	//top right		
		 0.5f, -0.5f, -0.5f,	1.0f, 0.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
		 0.5f, -0.5f,  0.5f,	1.0f, 0.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
		 0.5f,  0.5f,  0.5f,	1.0f, 0.0f, 0.0f,    0.0f, 1.0f,	//top left		

		 //left
		 -0.5f,  0.5f,  0.5f,	-1.0f, 0.0f, 0.0f,	 1.0f, 1.0f,	//top right		
		 -0.5f, -0.5f,  0.5f,	-1.0f, 0.0f, 0.0f,	 1.0f, 0.0f,	//bottom right	
		 -0.5f, -0.5f, -0.5f,	-1.0f, 0.0f, 0.0f,	 0.0f, 0.0f,	//bottom left	
		 -0.5f,  0.5f, -0.5f,	-1.0f, 0.0f, 0.0f,   0.0f, 1.0f,	//top left		
	};

	std::vector<unsigned int> cubeIndicies = 
	{
		//front
		0,  1,  2, //first triangle  
		2,  3,  0, //2nd triangle	   
		//back					   
		4,  5,  6,
		6,  7,  4,
		//up
		8,  9,  10,
		10, 11,  8,
		//down
		12, 13, 14,
		14, 15, 12,
		////right
		16, 17, 18,
		18, 19, 16,
		//left
		20, 21, 22,
		22, 23, 20,
	};

	//8 floats per vertex => (3 pos + 3 normals + 2 text coords)
	unsigned int floatsPerVertex = 8;
	unsigned int vertexCount = sizeof(cubeVertices) / (sizeof(float) * floatsPerVertex);

	std::vector<TextureItem> defaultTextures;
	auto texture1 = ResourceManager::GetTextureData("cube_container");
	auto texture2 = ResourceManager::GetTextureData("cube_chad");

	if (texture1)
		defaultTextures.push_back({ texture1, "texture_diffuse" });

	if(texture2)
		defaultTextures.push_back({ texture2, "texture_specular" });

	return std::make_unique<Mesh>
	(
		cubeVertices,
		sizeof(cubeVertices),
		cubeIndicies,
		defaultTextures,
		VertexLayouts::GetStaticLayout()
	);
}