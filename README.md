# Orbiter MSH Compiler
The purpose of this program is to compile [Orbiter](https://github.com/orbitersim/orbiter) `.MSH` files into a binary format.  This uses part of the orbiter core to read the `MSH` file and output a `CMSH` (compiled MSH) file.

## Usage

`mshcmp [options] [-i] <input_file> [-o <output_file>]`

| Cmd Param | Description |
| --------- | ----------- |
| `-i`      | Optional parameter to specify input file.  If used, the next parameter must be the name of the input mesh.  If omitted, the first argument that looks like a file name will be used. |
| `-o`      | Optional parameter to specify the output file.  If used, the next parameter must be the name of the output mesh.  If omitted, the second argument that looks like a file name will be used.  If the output file is omitted altogether, then the input file name will be used as the name of the output file, but will be given a `cmsh` extension. |
| `-s`      | Straight conversion of the msh file.  If used, the vertex components (position, normal, and UV coords) will be written to a single array.  If omitted, each component will be written to its own array. |
| `-m`      | Do not preserve material names.  If used, the material names will not be written to the output file. |

More options may be coming soon.

## Binary Format

### cmsh_header
The header for the compiled MSH file.  This structure is proceeded immediately by an array of `cmsh_group` or `cmsh_group_comp` structures, followed by an array of `cmsh_material` structures, followed by an array of `cmsh_texture` structures.

```c++
struct cmsh_header
{
	char Header[8];
	int GroupCount;
	int MaterialCount;
	int TextureCount;
	int VertexComponents : 1;
	int MaterialNames : 1;
};
```
#### Header
An 8-byte identification strng to identify the file as a compiled MSH file.  This is always `_CMSHX1_` and is not null-terminated.

#### GroupCount
The number of individual mesh groups in `CMSH` the file.

#### MaterialCount
The number of materials in the `CMSH` file.

#### TextureCount
The number of textures in the `CMSH` file.

#### VertexComponents
If `1`, each of the vertex components are in their seperate arrays.  If `0`, then the components are in a single array.  In future versions of this program, this may become a bitfield or split up into multiple fields.  It is advisable that you `and` this with `1` to see what the vertex layout is.

#### MaterialNames
If `1`, the material list is an array of `cmsh_material_named` structures.  If `0`, the material list is an array of `cmsh_material` structures.

### vtx2
```c++
struct vtx2 { float x, y; };
```
A 2-dimensional vertex element used only for the UV texture coords.

### vtx3
```c++
struct vtx3 { float x, y, z; };
```
A 3-dimensional vertex element used for the vertex position and normal components.

### vtx9
```c++
struct vtx9 { float x, y, z, nx, ny, nz, tu, tv; };
```
A vertex with all three components in one structure.  This is the layout that Orbiter represents mesh vertices in.  The position is the first 3 elements, the normal is the second 3 elements, and the UV coords are the last 2 elements.

### cmsh_group
When `cmsh_header::VertexComponents` is `0`, then an array of `cmsh_group` structures is generated, one for each mesh group.
```c++
struct cmsh_group
{
	int NameLength;
	char Name[NameLength];
	int MaterialIndex;
	int TextureIndex;
	unsigned Flags;
	unsigned UserFlags;
	unsigned ZBias;
	int VertexCount;
	int IndexCount;
	vtx9 VertexList[VertexCount];
	int IndexList[IndexCount];
};
```

#### NameLength
The length of the mesh group's name, including the null terminator.

#### Name
The name (or label) of the mesh group.  Must be null-terminated.

#### MaterialIndex
The index of the material to use.

#### TextureIndex
The index of the texture to use.

#### Flags
Mesh flags to use on the mesh.  See the Orbiter SDK documentation for more information.

#### UserFlags
User-defined flags for this mesh group.  See Orbiter SDK documentation for more information.

#### ZBias
See Orbiter SDK documentation for more information.

#### VertexCount
The number of vertices in the mesh group.

#### IndexCount
The number of indices in the mesh group.

#### VertexList
An array of `vtx9` structures containing position, normal, and UV coord data for each vertex.

#### IndexList
An array of integers containing index data.

### cmsh_group_comp
When `cmsh_header::VertexComponents` is `1`, then an array of `cmsh_group_comp` structures is generated, one for each mesh group.
```c++
struct cmsh_group_comp
{
	int NameLength;
	char Name[NameLength];
	int MaterialIndex;
	int TextureIndex;
	unsigned Flags;
	unsigned UserFlags;
	unsigned ZBias;
	int VertexCount;
	int IndexCount;
	vtx3 PositionList[VertexCount];
	vtx3 NormalList[VertexCount];
	vtx2 UVCoordList[VertexCount];
	int IndexList[IndexCount];
};
```
#### PositionList
An array of `vtx3` structures containing position data for each vertex.

#### NormalList
An array of `vtx3` structures containing normal data for each vertex.

#### UVCoordList
An array of `vtx2` structures containing UV coordinate data for each vertex.

### cmsh_material
An array of `cmsh_material` structures is generated after the array of `cmsh_group` or `cmsh_group_comp` structures if `cmsh_header::MaterialNames` is `0`.
```c++
struct cmsh_material
{
	float Diffuse[4];
	float Ambient[3];
	float Specular[3];
	float Emissive[3];
	float Power;
};
```

#### Diffuse
This is the color that the mesh will have if no texture is used.  If a texture is used, then the color of each pixel in the texture is multiplied by this color.

#### Ambient
See Orbiter SDK documentation for more information.

#### Specular
See Orbiter SDK documentation for more information.

#### Emissive
This causes the mesh to have a glow-in-the-dark effect.  The higher this value is, the brighter the mesh appears.

#### Power
See Orbiter SDK documentation for more information.

### cmsh_material_named
An array of `cmsh_material_named` structures is generated after the array of `cmsh_group` or `cmsh_group_comp` structures if `cmsh_header::MaterialNames` is `1`.
```c++
struct cmsh_material_named
{
	int NameLength;
	char Name[NameLength];
	float Diffuse[4];
	float Ambient[3];
	float Specular[3];
	float Emissive[3];
	float Power;
};
```

#### NameLength
The length of the material name including the null terminator.

#### Name
The name of the material.  Must be null-terminated.

### cmsh_texture
An array of `cmsh_texture` structures is generated after the array of `cmsh_material` structures.
```c++
struct cmsh_texture
{
	int NameLength;
	char Name[NameLength];
};
```

#### NameLength
The length of the file name for this texture, including the null-terminator.

#### Name
The name of the file to use for this texture.  Must be null-terminated.
