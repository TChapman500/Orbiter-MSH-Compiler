// Copyright (c) Martin Schweiger
// Licensed under the MIT License

// =======================================================================
// Classes to support meshes under D3D:
// Triangle, Mesh
// =======================================================================

#ifndef __MESH_H
#define __MESH_H

#define OAPI_IMPLEMENTATION
#include <d3d.h>
#include <d3dtypes.h>
#include <iostream>
//#include "OrbiterAPI.h"

/**
 * \ingroup structures
 * \brief vertex definition including normals and texture coordinates
 */
typedef struct {
	float x;     ///< vertex x position
	float y;     ///< vertex y position
	float z;     ///< vertex z position
	float nx;    ///< vertex x normal
	float ny;    ///< vertex y normal
	float nz;    ///< vertex z normal
	float tu;    ///< vertex u texture coordinate
	float tv;    ///< vertex v texture coordinate
} NTVERTEX;

typedef char Str256[256];

const DWORD SPEC_DEFAULT = (DWORD)(-1); // "default" material/texture flag
const DWORD SPEC_INHERIT = (DWORD)(-2); // "inherit" material/texture flag

// =======================================================================
// Class Triangle
// triangular surface patch

class Triangle {
public:
	Triangle ();
	Triangle (const Triangle &tri);
	Triangle (int n0, int n1, int n2);

	void SetNodes (int n0, int n1, int n2);

	int nd[3];  // node index list
	int nm[3];  // normal index list

	bool hasNodes;   // true if nd[] is valid
	bool hasNormals; // true if nm[] is valid
};

// =======================================================================
// mesh group descriptor

typedef struct {
	NTVERTEX  *Vtx;
	WORD      *Idx;
	DWORD     nVtx;
	DWORD     nIdx;
	DWORD     MtrlIdx;
	DWORD     TexIdx;
	DWORD     UsrFlag;
	WORD      zBias;
	WORD      Flags;
} GroupSpec;

// =======================================================================
// Class Mesh

class Mesh {
public:
	Mesh ();
	// Create an empty mesh

	Mesh (NTVERTEX *vtx, DWORD nvtx, WORD *idx, DWORD nidx,
		DWORD matidx = SPEC_DEFAULT, DWORD texidx = SPEC_DEFAULT);
	// Create a single-group mesh

	Mesh (const Mesh &mesh);
	// copy constructor

	~Mesh ();

	void Set (const Mesh &mesh);

	void Setup ();
	// call after all groups are assembled or whenever groups change,
	// to set up group parameters

	DWORD GetFlags () const { return Flags; }
	void SetFlags (DWORD flags) { Flags = flags; }

	void SetupGroup (DWORD grp);
	// Re-apply setup for a particular group (e.g. after transformation)

	inline DWORD nGroup() const { return nGrp; }
	// Number of groups

	inline DWORD nMaterial() const { return nMtrl; }
	// Number of materials

	inline DWORD nTexture() const { return nTex; }
	// Number of textures

	inline GroupSpec *GetGroup (DWORD grp) { return (grp < nGrp ? Grp+grp : 0); }
	// return a pointer to the group specification for group grp

	inline DWORD GetGroupUsrFlag (DWORD grp) const { return (grp < nGrp ? Grp[grp].UsrFlag : 0); }
	// return the user-defined flag for group grp

	int AddGroup (NTVERTEX *vtx, DWORD nvtx, WORD *idx, DWORD nidx,
		DWORD mtrl_idx = SPEC_INHERIT, DWORD tex_idx = SPEC_INHERIT,
		WORD zbias = 0, DWORD flag = 0, bool deepcopy = false);
	// Add new group to the mesh and return its group index
	// The lists are handled by the mesh and should not be released by
	// the calling program

	bool AddGroupBlock (DWORD grp, const NTVERTEX *vtx, DWORD nvtx, const WORD *idx, DWORD nidx);
	// Add geometry (vertices and indices) to an existing group.
	// Indices (idx) are zero-based. When adding them to the group, index
	// offsets are added automatically.

	bool DeleteGroup (DWORD grp);
	// Delete group 'grp'. Note that the indices of the other
	// groups may change as a result.
	// Return value is false if grp index is out of range

	bool MakeGroupVertexBuffer (DWORD grp);
	// copy the group vertex information into a vertex buffer in video memory
	// Ignored if the device has no T&L capability

	void AddMesh (Mesh &mesh);
	// Merge "mesh" into "this", by adding all groups of "mesh"
	// Currently this does not use the materials and textures of "mesh"

	inline D3DMATERIAL7 *GetMaterial (DWORD matidx)
	{ return (matidx < nMtrl ? Mtrl+matidx : 0); }
	// return a material pointer

	int AddMaterial (D3DMATERIAL7 &mtrl);
	// Add new material to the mesh and return its list index

	bool DeleteMaterial (DWORD matidx);
	// Delete material with index 'matidx' from the list. Any groups
	// using that material are reset to material 0. Any group material
	// indices > matidx are decremented to account for changed list

	inline char *GetTextureName(DWORD texidx)
	{
		if (texidx < nTex) return TexFiles[texidx].File;
		else return nullptr;
	}

	void SetTexMixture (DWORD grp, DWORD ntex, float mix);
	void SetTexMixture (DWORD ntex, float mix);

	void ScaleGroup (DWORD grp, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz);
	void Scale (D3DVALUE sx, D3DVALUE sy, D3DVALUE sz);
	// scale an individual group or the whole mesh

	void TranslateGroup (DWORD grp, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz);
	void Translate (D3DVALUE dx, D3DVALUE dy, D3DVALUE dz);
	// translate an individual group or the whole mesh

	enum RotAxis { ROTATE_X, ROTATE_Y, ROTATE_Z };
	void RotateGroup (DWORD grp, RotAxis axis, D3DVALUE angle);
	void Rotate (RotAxis axis, D3DVALUE angle);
	// rotate the mesh 'angle' rad around a coordiate axis

	void TransformGroup (DWORD grp, const D3DMATRIX &mat);
	void Transform (const D3DMATRIX &mat);
	// rotate mesh using the provided rotation matrix

	void TexScaleGroup (DWORD grp, D3DVALUE su, D3DVALUE sv);
	void TexScale (D3DVALUE su, D3DVALUE sv);
	// scale the texture coordinates of an individual group or the whole mesh

	void CalcNormals (DWORD grp, bool missingonly);
	// automatic calculation of vertex normals for group grp
	// if missingonly=true then only normals with zero length are calculated

	void CalcTexCoords (DWORD grp);
	// under construction

	void Clear ();

	static void GlobalEnableSpecular (bool enable);
	void EnableMatAlpha (bool enable);

	friend std::istream &operator>> (std::istream &is, Mesh &mesh);
	// read mesh from file

	struct tex_file
	{
		char File[256];
	};
	tex_file *Labels;
private:

	DWORD nGrp;         // number of groups
	GroupSpec *Grp;     // list of group specs	

	DWORD nMtrl;        // number of materials
	D3DMATERIAL7 *Mtrl; // list of materials used by the mesh

	DWORD nTex;         // number of textures
	tex_file *TexFiles;

	bool GrpSetup;      // true if the following arrays are allocated
	D3DVECTOR *GrpCnt;  // list of barycentres for each group (local coords)
	D3DVALUE *GrpRad;   // list of max. radii for each group
	DWORD *GrpVis;      // visibility flags for each group

	// global mesh flags
	static bool bEnableSpecular;   // enable specular reflection
	bool bModulateMatAlpha;
	// modulate material alpha with texture alpha (if disabled, any groups
	// that use textures ignore the material alpha values)
	DWORD Flags;        // bit | effect
	                    // 0   | set: mesh casts shadow (used for base object meshes)
						// 1   | set: use global shadow flag (bit 0) for all groups; unset: use individual group flags
};

// =======================================================================
// Class MeshManager: globally managed meshes

// =======================================================================
// Nonmember functions

void CreateSpherePatch (Mesh &mesh, int nlng, int nlat, int ilat, int res,
	int bseg = -1, bool reduce = true, bool outside = true);
// Create a mesh representing a rectangular patch on a sphere at a given
// position and resolution

#endif // !__MESH_H