#include "Core/SuntaPreCompiled.h"
#include "Primitives.h"

#include "Mesh.h"
#include "VertexTypes.h"
#include "VertexLayouts.h"
#include "Core/ResourceManager.h"
#include "RendererDevice.h"

namespace Sunta
{

class TextureItem;
	
std::unique_ptr<Mesh> Primitives::CreateCube(RendererDevice& rendererDevice)
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
	
	// TODO: Change indices to compatible with Face Culling (Backface Culling)
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
	
	/*std::vector<TextureItem> defaultTextures;
	auto texture1 = ResourceManager::GetTextureData("cube_container");
	auto texture2 = ResourceManager::GetTextureData("cube_chad");

	if (texture1)
		defaultTextures.push_back({ texture1, "texture_diffuse" });

	if(texture2)
		defaultTextures.push_back({ texture2, "texture_specular" });*/
	
	return std::make_unique<Mesh>
	(
		rendererDevice,
		cubeVertices,
		sizeof(cubeVertices),
		std::move(cubeIndicies),
		VertexLayouts::GetStaticLayout()
	);
}

std::unique_ptr<Mesh> Primitives::CreatePyramide(RendererDevice& rendererDevice)
{
	// Normal component ratio (Y:Z) is 1:2, unnormalized normal is (0, 1, 2)
	// Vector length = sqrt(0^2 + 1^2 + 2^2) = sqrt(5)
	constexpr float pyramidNormalY = 0.447f; // 1 / sqrt(5) 
	constexpr float pyramidNormalZ = 0.894f; // 2 / sqrt(5) 

	float pyramidVertices[] =
	{
		//positions							//normals								 //textures
		//front
		 0.0f,  0.5f,  0.0f,	     0.0f,		pyramidNormalY,  pyramidNormalZ,	 0.5f, 1.0f,	//top	
		-0.5f, -0.5f,  0.5f,	     0.0f,		pyramidNormalY,  pyramidNormalZ,	 0.0f, 0.0f,	//bottom left	
		 0.5f, -0.5f,  0.5f,	     0.0f,		pyramidNormalY,  pyramidNormalZ,	 1.0f, 0.0f,	//bottom right	

		//back
		 0.0f,  0.5f,  0.0f,	     0.0f,		pyramidNormalY, -pyramidNormalZ,	 0.5f, 1.0f,	//top	
		 0.5f, -0.5f, -0.5f,	     0.0f,		pyramidNormalY, -pyramidNormalZ,	 0.0f, 0.0f,	//bottom left	
		-0.5f, -0.5f, -0.5f,	     0.0f,		pyramidNormalY, -pyramidNormalZ,	 1.0f, 0.0f,	//bottom right	

		//right
		 0.0f,  0.5f,  0.0f,	pyramidNormalZ, pyramidNormalY,      0.0f,			 0.5f, 1.0f,	//top
		 0.5f, -0.5f,  0.5f,	pyramidNormalZ, pyramidNormalY,      0.0f,			 0.0f, 0.0f,	//bottom left	
		 0.5f, -0.5f, -0.5f,	pyramidNormalZ, pyramidNormalY,      0.0f,			 1.0f, 0.0f,	//bottom right	

		 //left
		 0.0f,  0.5f,  0.0f,   -pyramidNormalZ, pyramidNormalY,      0.0f,			 0.5f, 1.0f,	//top
		-0.5f, -0.5f, -0.5f,   -pyramidNormalZ, pyramidNormalY,      0.0f,			 0.0f, 0.0f,	//bottom left	
		-0.5f, -0.5f,  0.5f,   -pyramidNormalZ, pyramidNormalY,      0.0f,			 1.0f, 0.0f,	//bottom right	

		 //base
		 0.5f, -0.5f, -0.5f,	    0.0f,			-1.0f,			 0.0f,			 1.0f, 1.0f,	//top right		
		 0.5f, -0.5f,  0.5f,	    0.0f,			-1.0f,			 0.0f,			 1.0f, 0.0f,	//bottom right	
		-0.5f, -0.5f,  0.5f,	    0.0f,			-1.0f,			 0.0f,			 0.0f, 0.0f,	//bottom left	
		-0.5f, -0.5f, -0.5f,	    0.0f,			-1.0f,			 0.0f,			 0.0f, 1.0f,	//top left
	};

	std::vector<unsigned int> pyramidIndicies =
	{
		
		 0,  1,   2, // front
		 3,  4,   5, // back
		 6,  7,   8, // right
		 9,  10, 11, // left
		12,  15, 14, // base triangle 1
		12,  14, 13, // base triangle 2
	};

	//8 floats per vertex => (3 pos + 3 normals + 2 text coords)
	unsigned int floatsPerVertex = 8;
	unsigned int vertexCount = sizeof(pyramidVertices) / (sizeof(float) * floatsPerVertex);

	/*std::vector<TextureItem> defaultTextures;
	auto texture1 = ResourceManager::GetTextureData("cube_container");
	auto texture2 = ResourceManager::GetTextureData("cube_chad");

	if (texture1)
		defaultTextures.push_back({ texture1, "texture_diffuse" });

	if(texture2)
		defaultTextures.push_back({ texture2, "texture_specular" });*/

	return std::make_unique<Mesh>
	(
		rendererDevice,
		pyramidVertices,
		sizeof(pyramidVertices),
		std::move(pyramidIndicies),
		VertexLayouts::GetStaticLayout()
	);
}

}