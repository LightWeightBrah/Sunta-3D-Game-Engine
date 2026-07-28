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
	//8 floats per vertex => (3 pos + 3 normals + 2 text coords)

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
	std::vector<unsigned int> cubeIndices = 
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
	
	return std::make_unique<Mesh>
	(
		rendererDevice,
		cubeVertices,
		sizeof(cubeVertices),
		std::move(cubeIndices),
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

	std::vector<unsigned int> pyramidIndices =
	{
		
		 0,  1,   2, // front
		 3,  4,   5, // back
		 6,  7,   8, // right
		 9,  10, 11, // left
		12,  15, 14, // base triangle 1
		12,  14, 13, // base triangle 2
	};

	return std::make_unique<Mesh>
	(
		rendererDevice,
		pyramidVertices,
		sizeof(pyramidVertices),
		std::move(pyramidIndices),
		VertexLayouts::GetStaticLayout()
	);
}


// ========================================================
//														  |
// See visual representation of math at following page:	  |
// https://www.songho.ca/opengl/gl_sphere.html			  |
//														  |
// ========================================================
std::unique_ptr<Mesh> Primitives::CreateSphere(RendererDevice& rendererDevice)
{
	std::vector<float> sphereVertices;
	std::vector<unsigned int> sphereIndices;

	const float PI = 3.14159265359;

	float radius = 0.5f;

	unsigned int stackCount = 32;  // horizontal rings (top - down)
	unsigned int sectorCount = 64; // points on every ring (circle)

	// Step size in radians for each loop iteration
	// 2 * PI = 360 degrees (full circle horizontally)
	float sectorStep = 2 * PI / sectorCount;

	// PI = 180 degrees (from top pole to bottom pole)
	float stackStep = PI / stackCount;

	// =================================================
	//				VERTEX GENERATION				   |
	// =================================================

	// We are going from TOP POLE to BOTTOM POLE (VERTICAL)
	for (unsigned int i = 0; i <= stackCount; i++)
	{
		// Vertical angle from (+90 degrees AT TOP) to (-90 degrees AT BOTTOM)
		float stackAngle = (PI / 2.0f) - (stackStep * i);
		
		// Vertical position (Y axis):
		// sin(+90 deg) =  1 -> TOP POLE    (+radius)
		// sin(  0 deg) =  0 -> EQUATOR     (   0   )
		// sin(-90 deg) = -1 -> BOTTOM POLE (-radius)
		float y = radius * std::sin(stackAngle);

		// Radius of the current horizontal ring (slice of sphere)
		// cos(+90 deg) = 0 -> Ring radius = 0 (just a single point at TOP)
		// cos(  0 deg) = 1 -> Ring radius = MAXIMUM AT THE EQUATOR
		// cos(-90 deg) = 0 -> Ring radius = 0 (just a single point at BOTTOM)
		float ringRadius = radius * std::cos(stackAngle);

		// loop around the current horizontal ring (0 to 360 degrees)
		for (unsigned int j = 0; j <= sectorCount; j++)
		{
			// Horizontal angle (from 0 to 2Pi) (0 degrees to 360 degrees)
			float sectorAngle = sectorStep * j;

			// We take the ring radius and we divide it to X and Z:
			// X = width in left/right (cos of horizontal angle)
			// Z = depth in front/back (sin of horizontal angle)
			float x = ringRadius * std::cos(sectorAngle);
			float z = ringRadius * std::sin(sectorAngle);

			// Surface direction (normal vector)
			// Since sphere center is at (0, 0, 0), normal is just position divided by radius
			float normalX = x / radius;
			float normalY = y / radius;
			float normalZ = z / radius;

			float u = (float)j / sectorCount;
			float v = (float)i / stackCount;

			sphereVertices.insert(sphereVertices.end(), {x, y, z, normalX, normalY, normalZ, u, v});
		}
	}


	// ===============================================
	//				INDEX GENERATION				 |
	// ===============================================
	for (unsigned int i = 0; i < stackCount; i++)
	{
		// Each row has (sectorCount + 1) vertices because the last vertex
		// overlaps the first vertex to complete the texture (UV: 0.0 -> 1.0)
		unsigned int k1 = i  * (sectorCount + 1);		// start of current row
		unsigned int k2 = k1 + (sectorCount + 1);		// start of row below

		for (unsigned int j = 0; j < sectorCount; j++, k1++, k2++)
		{
			// Triangles to connect
			//
			// k1 ------- k1 + 1       first row
			// |     	 /   |
			// |       /	 |
			// |	 /       |
			// |   /      	 |
			// | /      	 |
			// k2 ------- k2 + 1       row below


			// Triangle 1: Top-left (k1) -> Top-right (k1 + 1) -> Bottom-left (k2)
			// 
			// SKIP at TOP POLE (i == 0) cause 2 vertices
			// are the exact same point (0, radius, 0) and that triangle would have 0 area
			if (i != 0)
			{
				sphereIndices.push_back(k1);
				sphereIndices.push_back(k1 + 1);
				sphereIndices.push_back(k2);
			}

			// Triangle 2: Bottom-left (k2) -> Top-right (k1 + 1) -> Bottom-right(k2 + 1)
			// 
			// SKIP at BOTTOm POLE (i == stackCount - 1) cause 2 vertices
			// are the exact same point (0, -radius, 0) and that triangle would have 0 area
			if (i != (stackCount - 1))
			{
				sphereIndices.push_back(k2);
				sphereIndices.push_back(k1 + 1);
				sphereIndices.push_back(k2 + 1);
			}
		}
	}
	

	

	//8 floats per vertex => (3 pos + 3 normals + 2 text coords)
	unsigned int floatsPerVertex = 8;
	unsigned int vertexCount = sizeof(sphereVertices) / (sizeof(float) * floatsPerVertex);

	return std::make_unique<Mesh>
	(
		rendererDevice,
		sphereVertices.data(),
		static_cast<unsigned int>(sphereVertices.size() * sizeof(float)),
		std::move(sphereIndices),
		VertexLayouts::GetStaticLayout()
	);
}

}