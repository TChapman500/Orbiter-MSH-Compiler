#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Mesh.h"

struct vtx9 { float x, y, z, nx, ny, nz, tu, tv; };
struct vtx3 { float x, y, z; };
struct vtx2 { float x, y; };

class ExMeshGroup
{
public:
	char Label[256];
	int MaterialIndex;
	int TextureIndex;
	unsigned Flags;
	unsigned UserFlags;
	unsigned ZBias;

	int VertexCount;
	int IndexCount;

	vtx3 *Positions;
	vtx3 *Normals;
	vtx2 *UVCoords;
	int *Indices;

public:
	ExMeshGroup(GroupSpec *parent, char *label)
	{
		ZeroMemory(Label, 256);
		if (label)
		{
			int len = strlen(label);
			for (int i = 0; i < len; i++) Label[i] = label[i];
		}

		MaterialIndex = 0;
		TextureIndex = 0;
		Flags = 0;
		UserFlags = 0;
		ZBias = 0;

		VertexCount = 0;
		IndexCount = 0;

		Positions = nullptr;
		Normals = nullptr;
		UVCoords = nullptr;
		Indices = nullptr;

		if (!parent) return;

		// Copy parent material data.
		MaterialIndex = (int)parent->MtrlIdx;
		TextureIndex = (int)parent->TexIdx;
		Flags = (unsigned)parent->Flags;
		UserFlags = (unsigned)parent->UsrFlag;
		ZBias = (unsigned)parent->zBias;

		// Copy vertex and index data.
		VertexCount = parent->nVtx;
		IndexCount = parent->nIdx;

		// Allocate room for vertices.
		Positions = new(std::nothrow) vtx3[VertexCount];
		Normals = new(std::nothrow) vtx3[VertexCount];
		UVCoords = new(std::nothrow) vtx2[VertexCount];

		if (!Positions || !Normals || !UVCoords)
		{
			if (Positions) delete[] Positions;
			if (Normals) delete[] Normals;
			if (UVCoords) delete[] UVCoords;
			Positions = nullptr;
			Normals = nullptr;
			UVCoords = nullptr;
			return;
		}

		// Populate vertex data.
		for (int i = 0; i < VertexCount; i++)
		{
			Positions[i].x = parent->Vtx[i].x;
			Positions[i].y = parent->Vtx[i].y;
			Positions[i].z = parent->Vtx[i].z;
			Normals[i].x = parent->Vtx[i].nx;
			Normals[i].y = parent->Vtx[i].ny;
			Normals[i].z = parent->Vtx[i].nz;
			UVCoords[i].x = parent->Vtx[i].tu;
			UVCoords[i].y = parent->Vtx[i].tv;
		}

		// Allocate room for indices.
		Indices = new(std::nothrow) int[IndexCount];
		if (!Indices)
		{
			delete[] Positions;
			Positions = nullptr;
			delete[] Normals;
			Normals = nullptr;
			delete[] UVCoords;
			UVCoords = nullptr;
			return;
		}

		for (int i = 0; i < IndexCount; i++)
		{
			// Mask-out the other bits just in case.
			Indices[i] = (int)parent->Idx[i] & 0xFFFF;
		}
	}

	bool Validate()
	{
		if (IndexCount && !Indices) return false;
		if (VertexCount && (!Positions || !Normals || !UVCoords)) return false;
		return true;
	}

	~ExMeshGroup()
	{
		if (Indices) delete[] Indices;
		if (Positions) delete[] Positions;
		if (Normals) delete[] Normals;
		if (UVCoords) delete[] UVCoords;
	}
};

class ExMaterial
{
public:
	char Name[256];
	float Diffuse[4];
	float Ambient[3];
	float Specular[3];
	float Emissive[3];
	float Power;

	ExMaterial(D3DMATERIAL7 *material, char *name)
	{
		ZeroMemory(this, sizeof(ExMaterial));

		if (!material) return;

		if (name)
		{
			int length = strlen(name);
			for (int i = 0; i < length; i++) Name[i] = name[i];
		}

		Diffuse[0] = material->diffuse.r;
		Diffuse[1] = material->diffuse.g;
		Diffuse[2] = material->diffuse.b;
		Diffuse[3] = material->diffuse.a;

		Ambient[0] = material->ambient.r;
		Ambient[1] = material->ambient.g;
		Ambient[2] = material->ambient.b;

		Specular[0] = material->specular.r;
		Specular[1] = material->specular.g;
		Specular[2] = material->specular.b;

		Emissive[0] = material->emissive.r;
		Emissive[1] = material->emissive.g;
		Emissive[2] = material->emissive.b;

		Power = material->power;
	}
};

class ExTexture
{
public:
	char Name[256];

	ExTexture(char *name)
	{
		ZeroMemory(Name, 256);
		unsigned length = strlen(name);
		for (int i = 0; i < length; i++) Name[i] = name[i];
	}
};

class ExMesh
{
public:
	int GroupCount;
	int MaterialCount;
	int TextureCount;

	ExMeshGroup **GroupList;
	ExMaterial **MaterialList;
	ExTexture **TextureList;
	
	ExMesh(Mesh *mesh)
	{
		GroupCount = 0;
		MaterialCount = 0;
		TextureCount = 0;

		GroupList = nullptr;
		MaterialList = nullptr;
		TextureList = nullptr;

		if (!mesh) return;
		bool failed = false;

		// Create Mesh Groups.
		GroupSpec newGroup;
		GroupCount = mesh->nGroup();
		if (GroupCount)
		{
			GroupList = new(std::nothrow) ExMeshGroup * [GroupCount];
			if (GroupList)
			{
				for (int i = 0; i < GroupCount; i++)
				{
					char *label = mesh->Labels[i].File;
					if (mesh->Labels[i].File[0] == '\0') label = nullptr;
					GroupList[i] = new(std::nothrow) ExMeshGroup(mesh->GetGroup(i), label);
				}
			}
		}

		// Create Materials
		MaterialCount = mesh->nMaterial();
		if (MaterialCount)
		{
			MaterialList = new(std::nothrow) ExMaterial * [MaterialCount];
			if (MaterialList)
			{
				for (int i = 0; i < MaterialCount; i++)
				{
					char *name = mesh->MatNames[i].File;
					if (name[0] == '\0') name = nullptr;
					MaterialList[i] = new(std::nothrow) ExMaterial(mesh->GetMaterial(i), name);
				}
			}
		}

		// Create Textures
		TextureCount = mesh->nTexture();
		if (TextureCount)
		{
			TextureList = new(std::nothrow) ExTexture * [TextureCount];
			if (TextureList)
			{
				for (int i = 0; i < TextureCount; i++)
				{
					TextureList[i] = new(std::nothrow) ExTexture(mesh->GetTextureName(i));
				}
			}
		}
	}

	bool Validate()
	{
		if (GroupCount && !GroupList) return false;
		if (MaterialCount && !MaterialList) return false;
		if (TextureCount && !TextureList) return false;
		
		for (int i = 0; i < GroupCount; i++)
			if (!GroupList[i]->Validate()) return false;

		return true;
	}

	~ExMesh()
	{
		if (GroupList)
		{
			for (int i = 0; i < GroupCount; i++)
				delete GroupList[i];
			delete[] GroupList;
		}
	}
};

struct cmsh_header
{
	char Header[8];
	int GroupCount;
	int MaterialCount;
	int TextureCount;
	int VertexComponents : 1;
	int MaterialNames : 1;
};


int main(int argCount, char **argList)
{
	char *inputFile = nullptr;
	char *outputFile = nullptr;

	bool straightConvert = false;
	bool inputNext = false;
	bool outputNext = false;
	bool noMatNames = false;

	for (int i = 1; i < argCount; i++)
	{
		if (!inputNext && !outputNext)
		{
			if (strcmp(argList[i], "-s") == 0) straightConvert = true;
			else if (strcmp(argList[i], "-i") == 0) inputNext = true;
			else if (strcmp(argList[i], "-o") == 0) outputNext = true;
			else if (strcmp(argList[i], "-m") == 0) noMatNames = true;
			else if (!inputFile) inputFile = argList[i];
			else if (!outputFile) outputFile = argList[i];
		}
		else
		{
			if (inputNext)
			{
				inputNext = false;
				inputFile = argList[i];
			}
			else if (outputNext)
			{
				outputNext = false;
				outputFile = argList[i];
			}
		}
	}

	if (!inputFile)
	{
		std::cout << "Usage:" << std::endl;
		std::cout << "\t-i:\tInput File" << std::endl;
		std::cout << "\t-o:\tOutput File" << std::endl;
		std::cout << "\t-m:\tDo Not Preserve Material Names" << std::endl;
		std::cout << "\t-s:\tAll Vertex Elements in Single Array" << std::endl << std::endl;
		return 0;
	}

	bool outputAllocated = false;
	if (!outputFile)
	{
		outputAllocated = true;

		// Length of input file name.
		int length = strlen(inputFile);

		// Index of decimal point.
		int index = -1;

		// Search for extension.
		for (int i = 0; i < length; i++)
		{
			if (inputFile[i] == '.')
			{
				index = i;
				break;
			}
		}

		// No extension found.
		if (index == -1)
		{
			// Set output file and designate extension.
			int outLength = length + 6;
			outputFile = new(std::nothrow) char[outLength];
			if (!outputFile)
			{
				std::cout << "Error:  Could not generate output file." << std::endl;
				return -1;
			}
			for (int i = 0; i < length; i++) outputFile[i] = inputFile[i];

			// Append mesh extension.
			outputFile[length + 0] = '.';
			outputFile[length + 1] = 'c';
			outputFile[length + 2] = 'm';
			outputFile[length + 3] = 's';
			outputFile[length + 4] = 'h';
			outputFile[length + 5] = '\0';
		}
		// Extension found.
		else
		{
			// The length of the extension.
			int extLength = length - index - 1;	// Extension length minus the decimal point.
			int outLength = length - extLength + 4;	// Output length.
			int lenNoExt = length - extLength + 1;	// Make sure to copy the decimal point.

			int finalLength = outLength + 1;
			outputFile = new(std::nothrow) char[finalLength];
			if (!outputFile)
			{
				std::cout << "Error:  Could not generate output file." << std::endl;
				return -1;
			}

			// Copy input file name without extension, but including the decimal point.
			for (int i = 0; i < lenNoExt; i++) outputFile[i] = inputFile[i];

			// Copy the extension.
			outputFile[outLength - 4] = 'c';
			outputFile[outLength - 3] = 'm';
			outputFile[outLength - 2] = 's';
			outputFile[outLength - 1] = 'h';
			outputFile[outLength - 0] = '\0';
		}
	}

	// Read Mesh File
	Mesh *iMesh = new(std::nothrow) Mesh();
	if (!iMesh)
	{
		std::cout << "Error:  Could not initialize mesh file for \"" << inputFile << "\"." << std::endl;
		if (outputAllocated) delete[] outputFile;
		return -2;
	}

	std::ifstream iMeshFile(inputFile);
	if (!iMeshFile.is_open())
	{
		std::cout << "Error:  Could not open \"" << inputFile << "\"." << std::endl;
		if (outputAllocated) delete[] outputFile;
		delete iMesh;
		return -3;
	}

	iMeshFile >> *iMesh;
	iMeshFile.close();

	// Convert Mesh File.
	ExMesh *oMesh = new(std::nothrow) ExMesh(iMesh);
	if (!oMesh)
	{
		std::cout << "Error:  Could not convert \"" << inputFile << "\" data." << std::endl;
		if (outputAllocated) delete[] outputFile;
		delete iMesh;
		return -4;
	}

	// Delete Mesh File
	delete iMesh;

	if (!oMesh->Validate())
	{
		std::cout << "Converted mesh failed validation." << std::endl;
		delete oMesh;
		return -10;
	}

	if (!oMesh->GroupList)
	{
		std::cout << "Error:  Could not convert \"" << inputFile << "\" data." << std::endl;
		if (outputAllocated) delete[] outputFile;
		return -4;
	}

	cmsh_header header;
	ZeroMemory(&header, sizeof(cmsh_header));
	header.Header[0] = '_';
	header.Header[1] = 'C';
	header.Header[2] = 'M';
	header.Header[3] = 'S';
	header.Header[4] = 'H';
	header.Header[5] = 'X';
	header.Header[6] = '1';
	header.Header[7] = '_';
	header.GroupCount = oMesh->GroupCount;
	header.MaterialCount = oMesh->MaterialCount;
	header.TextureCount = oMesh->TextureCount;
	header.VertexComponents = straightConvert ? 0 : 1;
	header.MaterialNames = noMatNames ? 0 : 1;

	std::cout << "Group Count:\t" << header.GroupCount << std::endl;
	std::cout << "Material Count:\t" << header.MaterialCount << std::endl;
	std::cout << "Texture Count:\t" << header.TextureCount << std::endl << std::endl;


	std::ofstream oMeshFile(outputFile, std::ios::binary);
	if (!oMeshFile.is_open())
	{
		std::cout << "Error:  Could not create \"" << outputFile << "\"." << std::endl;
		if (outputAllocated) delete[] outputFile;
		delete oMesh;
		return -5;
	}

	oMeshFile.write((char *)&header, sizeof(cmsh_header));

	// Write mesh group data.
	if (oMesh->GroupList)
	{
		for (int i = 0; i < oMesh->GroupCount; i++)
		{
			// The current mesh group.
			ExMeshGroup *current = oMesh->GroupList[i];

			std::cout << "Mesh Group: " << current->Label << std::endl;
			std::cout << "\tMat Index:\t" << current->MaterialIndex << std::endl;
			std::cout << "\tTexture Index:\t" << current->TextureIndex << std::endl;
			std::cout << "\tVertex Count:\t" << current->VertexCount << std::endl;
			std::cout << "\tIndex Count:\t" << current->IndexCount << std::endl << std::endl;

			// Write group header.
			int length = strlen(current->Label) + 1;
			oMeshFile.write((char *)&length, 4);
			if (length) oMeshFile.write(current->Label, length);
			oMeshFile.write((char *)&current->MaterialIndex, 4);
			oMeshFile.write((char *)&current->TextureIndex, 4);
			oMeshFile.write((char *)&current->Flags, 4);
			oMeshFile.write((char *)&current->UserFlags, 4);
			oMeshFile.write((char *)&current->ZBias, 4);
			oMeshFile.write((char *)&current->VertexCount, 4);
			oMeshFile.write((char *)&current->IndexCount, 4);

			// Write vertex data (all components in one array).
			if (straightConvert)
			{
				for (int v = 0; v < current->VertexCount; v++)
				{
					oMeshFile.write((char *)&current->Positions[v], 12);
					oMeshFile.write((char *)&current->Normals[v], 12);
					oMeshFile.write((char *)&current->UVCoords[v], 8);
				}
			}
			// Write vertex data (each component in separate array).
			else
			{
				int posBytes = 12 * current->VertexCount;
				int texBytes = 8 * current->VertexCount;
				oMeshFile.write((char *)current->Positions, posBytes);
				oMeshFile.write((char *)current->Normals, posBytes);
				oMeshFile.write((char *)current->UVCoords, texBytes);
			}

			// Write index data.
			int iBytes = 4 * current->IndexCount;
			oMeshFile.write((char *)current->Indices, iBytes);
		}
	}

	// Write material data.
	if (oMesh->MaterialList)
	{
		for (int i = 0; i < oMesh->MaterialCount; i++)
		{
			ExMaterial *current = oMesh->MaterialList[i];

			// Preserve material names.
			if (header.MaterialNames)
			{
				int length = strlen(current->Name) + 1;
				oMeshFile.write((char *)&length, 4);
				oMeshFile.write((char *)current->Name, length);
			}

			// Write the rest of the file.
			oMeshFile.write((char *)current->Diffuse, 16);
			oMeshFile.write((char *)current->Ambient, 12);
			oMeshFile.write((char *)current->Specular, 12);
			oMeshFile.write((char *)current->Emissive, 12);
			oMeshFile.write((char *)&current->Power, 4);
		}
	}

	// Write texture data.
	if (oMesh->TextureList)
	{
		for (int i = 0; i < oMesh->TextureCount; i++)
		{
			ExTexture *current = oMesh->TextureList[i];
			int length = strlen(current->Name) + 1;
			oMeshFile.write((char *)&length, 4);
			oMeshFile.write(current->Name, length);
		}
	}

	oMeshFile.close();
	delete oMesh;
	if (outputAllocated) delete[] outputFile;

	return 0;
}
