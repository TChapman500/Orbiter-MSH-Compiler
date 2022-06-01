// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "Mesh.h"
#include <stdio.h>
#include <d3dtypes.h>
#include "d3dmath.h"

#ifdef INLINEGRAPHICS
#include "OGraphics.h"
#include "Texture.h"
#include "Scene.h"
extern TextureManager *g_texmanager;
#endif // INLINEGRAPHICS

using namespace std;

extern DWORD g_vtxcount;
extern char DBG_MSG[256];

static D3DMATERIAL7 defmat = {{1,1,1,1},{1,1,1,1},{0,0,0,1},{0,0,0,1},0};

// =======================================================================
// Class Triangle

Triangle::Triangle ()
{
	hasNodes = false;
	hasNormals = false;
}

Triangle::Triangle (const Triangle &tri)
{
	if (hasNodes = tri.hasNodes)
		for (int i = 0; i < 3; i++) nd[i] = tri.nd[i];
	if (hasNormals = tri.hasNormals)
		for (int i = 0; i < 3; i++) nm[i] = tri.nm[i];
}

Triangle::Triangle (int n0, int n1, int n2)
{
	hasNodes = true;
	hasNormals = false;
	nd[0] = n0;
	nd[1] = n1;
	nd[2] = n2;
}

void Triangle::SetNodes (int n0, int n1, int n2)
{
	hasNodes = true;
	nd[0] = n0;
	nd[1] = n1;
	nd[2] = n2;
}

// =======================================================================
// Class Mesh

Mesh::Mesh ()
{
	nGrp = nMtrl = nTex = 0;
	GrpVis   = 0;
	GrpSetup = false;
	bModulateMatAlpha = false;
}

Mesh::Mesh (NTVERTEX *vtx, DWORD nvtx, WORD *idx, DWORD nidx, DWORD matidx, DWORD texidx)
{
	nGrp = nMtrl = nTex = 0;
	GrpVis   = 0;
	GrpSetup = false;
	AddGroup (vtx, nvtx, idx, nidx, matidx, texidx);
	bModulateMatAlpha = false;
	Setup();
}

Mesh::Mesh (const Mesh &mesh)
{
	nGrp = nMtrl = nTex = 0;
	GrpVis = 0;
	GrpSetup = false;
	Set (mesh);
}

void Mesh::Set (const Mesh &mesh)
{
	DWORD i;

	Clear ();
	if (nGrp = mesh.nGrp) {
		Grp = new GroupSpec[nGrp];
		memcpy (Grp, mesh.Grp, nGrp*sizeof(GroupSpec));
		for (i = 0; i < nGrp; i++) {
			Grp[i].Vtx = new NTVERTEX[Grp[i].nVtx];
			memcpy (Grp[i].Vtx, mesh.Grp[i].Vtx, Grp[i].nVtx*sizeof(NTVERTEX));
			Grp[i].Idx = new WORD[Grp[i].nIdx];
			memcpy (Grp[i].Idx, mesh.Grp[i].Idx, Grp[i].nIdx*sizeof(WORD));
		}
	}
	if (nMtrl = mesh.nMtrl) {
		Mtrl = new D3DMATERIAL7[nMtrl];
		memcpy (Mtrl, mesh.Mtrl, nMtrl*sizeof(D3DMATERIAL7));
	}
	if (nTex = mesh.nTex) {
	}
	if (GrpSetup = mesh.GrpSetup) {
		GrpCnt = new D3DVECTOR[nGrp];
		memcpy (GrpCnt, mesh.GrpCnt, nGrp*sizeof(D3DVECTOR));
		GrpRad = new D3DVALUE[nGrp];
		memcpy (GrpRad, mesh.GrpRad, nGrp*sizeof(D3DVALUE));
		GrpVis = new DWORD[nGrp];
		memcpy (GrpVis, mesh.GrpVis, nGrp*sizeof(DWORD));
	} else {
		GrpVis = 0;
	}
	bModulateMatAlpha = mesh.bModulateMatAlpha;
}

Mesh::~Mesh ()
{
	Clear ();
}

void Mesh::Setup ()
{
	DWORD g;
	if (GrpVis) { // allocated already
		delete []GrpCnt;
		delete []GrpRad;
		delete []GrpVis;
	}
	GrpCnt  = new D3DVECTOR[nGrp];
	GrpRad  = new D3DVALUE[nGrp];
	GrpVis  = new DWORD[nGrp];
	GrpSetup = true;
	for (g = 0; g < nGrp; g++) {
		SetupGroup (g);
		// check validity of material/texture indices
		if (Grp[g].MtrlIdx != SPEC_INHERIT && Grp[g].MtrlIdx >= nMtrl) Grp[g].MtrlIdx = SPEC_DEFAULT;
		if (Grp[g].TexIdx != SPEC_INHERIT && Grp[g].TexIdx >= nTex) Grp[g].TexIdx = SPEC_DEFAULT;
	}
}

void Mesh::SetupGroup (DWORD grp)
{
	DWORD i;
	D3DVALUE x, y, z, dx, dy, dz, d2, d2max;
	D3DVALUE invtx = (D3DVALUE)(1.0/Grp[grp].nVtx);
	x = y = z = 0.0f;
	for (i = 0; i < Grp[grp].nVtx; i++) {
		x += Grp[grp].Vtx[i].x;
		y += Grp[grp].Vtx[i].y;
		z += Grp[grp].Vtx[i].z;
	}
	GrpCnt[grp].x = (x *= invtx);
	GrpCnt[grp].y = (y *= invtx);
	GrpCnt[grp].z = (z *= invtx);
	d2max = 0.0f;
	for (i = 0; i < Grp[grp].nVtx; i++) {
		dx = x - Grp[grp].Vtx[i].x;
		dy = y - Grp[grp].Vtx[i].y;
		dz = z - Grp[grp].Vtx[i].z;
		d2 = dx*dx + dy*dy + dz*dz;
		if (d2 > d2max) d2max = d2;
	}
	GrpRad[grp] = (FLOAT)sqrt (d2max);
}

int Mesh::AddGroup (NTVERTEX *vtx, DWORD nvtx, WORD *idx, DWORD nidx,
	DWORD mtrl_idx, DWORD tex_idx, WORD zbias, DWORD flag, bool deepcopy)
{
	DWORD n;
	GroupSpec *g, *tmp_Grp = new GroupSpec[nGrp+1];
	D3DVECTOR *tmp_Cnt = new D3DVECTOR[nGrp+1];
	D3DVALUE *tmp_Rad = new D3DVALUE[nGrp+1];
	DWORD *tmp_Vis = new DWORD[nGrp+1];
	if (nGrp) {
		memcpy (tmp_Grp, Grp, nGrp*sizeof(GroupSpec));
		memcpy (tmp_Cnt, GrpCnt, nGrp*sizeof(D3DVECTOR));
		memcpy (tmp_Rad, GrpRad, nGrp*sizeof(D3DVALUE));
		memcpy (tmp_Vis, GrpVis, nGrp*sizeof(DWORD));
		delete []Grp;
		delete []GrpCnt;
		delete []GrpRad;
		delete []GrpVis;
	}
	Grp = tmp_Grp;
	GrpCnt = tmp_Cnt;
	GrpRad = tmp_Rad;
	GrpVis = tmp_Vis;
	g = Grp+nGrp;
	if (deepcopy) {
		g->Vtx = new NTVERTEX[nvtx]; 
		if (vtx) memcpy (g->Vtx, vtx, nvtx*sizeof(NTVERTEX));
		else     memset (g->Vtx, 0, nvtx*sizeof(NTVERTEX));
		g->Idx = new WORD[nidx];
		if (idx) memcpy (g->Idx, idx, nidx*sizeof(WORD));
		else     memset (g->Idx, 0, nidx*sizeof(WORD));
	} else {
		g->Vtx = vtx;
		g->Idx = idx;
	}
	g->nVtx = nvtx;
	g->nIdx = nidx;
	g->MtrlIdx = mtrl_idx;
	g->TexIdx = tex_idx;
	g->zBias = zbias;
	g->Flags = 0;
	g->UsrFlag = flag;
	if (GrpSetup) {
		SetupGroup (nGrp);
		if (g->MtrlIdx != SPEC_INHERIT && g->MtrlIdx >= nMtrl)
			g->MtrlIdx = SPEC_DEFAULT;
		if (g->TexIdx != SPEC_INHERIT && g->TexIdx >= nTex)
			g->TexIdx = SPEC_DEFAULT;
	}
	return nGrp++;
}

bool Mesh::AddGroupBlock (DWORD grp, const NTVERTEX *vtx, DWORD nvtx, const WORD *idx, DWORD nidx)
{
	if (grp >= nGrp) return false;

	GroupSpec *g = Grp+grp;
	WORD vofs = (WORD)g->nVtx;

	NTVERTEX *v = new NTVERTEX[g->nVtx+nvtx];
	if (g->nVtx) {
		memcpy (v, g->Vtx, g->nVtx*sizeof(NTVERTEX));
		delete []g->Vtx;
	}
	if (nvtx) {
		memcpy (v+g->nVtx, vtx, nvtx*sizeof(NTVERTEX));
	}
	g->Vtx = v;
	g->nVtx += nvtx;

	WORD *i = new WORD[g->nIdx+nidx];
	if (g->nIdx) {
		memcpy (i, g->Idx, g->nIdx*sizeof(WORD));
		delete []g->Idx;
	}
	if (nidx) {
		for (DWORD j = 0; j < nidx; j++)
			i[g->nIdx+j] = idx[j] + vofs;
	}
	g->Idx = i;
	g->nIdx += nidx;

	return true;
}

bool Mesh::MakeGroupVertexBuffer (DWORD grp)
{
#ifdef INLINEGRAPHICS
	GroupSpec &g = Grp[grp];
	if (g.VtxBuf) return false; // buffer already exists
	if (!g_pOrbiter->GetInlineGraphicsClient()->GetFramework()->IsTLDevice()) return false; // no T&L capability

	LPDIRECT3D7 d3d = g_pOrbiter->GetInlineGraphicsClient()->GetDirect3D7();
	LPDIRECT3DDEVICE7 dev = g_pOrbiter->GetInlineGraphicsClient()->GetDevice();
	LPVOID data;
	D3DVERTEXBUFFERDESC vbd = 
		{ sizeof(D3DVERTEXBUFFERDESC), D3DVBCAPS_WRITEONLY, D3DFVF_VERTEX, g.nVtx };
	if (d3d->CreateVertexBuffer (&vbd, &g.VtxBuf, 0) != D3D_OK) return false;
	LOGOUT_DDERR (g.VtxBuf->Lock (DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_DISCARDCONTENTS, (LPVOID*)&data, NULL));
	memcpy (data, g.Vtx, g.nVtx*sizeof(NTVERTEX));
	LOGOUT_DDERR (g.VtxBuf->Unlock());
	LOGOUT_DDERR (g.VtxBuf->Optimize (dev, 0));
	return true;
#else
	return false;
#endif
}

void Mesh::AddMesh (Mesh &mesh)
{
	for (DWORD i = 0; i < mesh.nGroup(); i++) {
		NTVERTEX *vtx2;
		WORD *idx2;
		GroupSpec *gs = mesh.GetGroup (i);
		// need to copy the vertex and index lists
		vtx2 = new NTVERTEX[gs->nVtx];  memcpy (vtx2, gs->Vtx, gs->nVtx*sizeof(NTVERTEX));
		idx2 = new WORD[gs->nIdx];      memcpy (idx2, gs->Idx, gs->nIdx*sizeof(WORD));
		AddGroup (vtx2, gs->nVtx, idx2, gs->nIdx);
	}
}

bool Mesh::DeleteGroup (DWORD grp)
{
	if (grp < nGrp) {
		delete []Grp[grp].Vtx;
		delete []Grp[grp].Idx;

		if (nGrp == 1) { // delete the only group
			delete []Grp;
		} else {
			GroupSpec *tmp_Grp = new GroupSpec[nGrp-1];
			for (DWORD i = 0, j = 0; i < nGrp; i++)
				if (i != grp) tmp_Grp[j++] = Grp[i];
			delete []Grp;
			Grp = tmp_Grp;
		}
		nGrp--;
		return true;
	} else {
		return false;
	}
}

int Mesh::AddMaterial (D3DMATERIAL7 &mtrl)
{
	D3DMATERIAL7 *tmp_Mtrl = new D3DMATERIAL7[nMtrl+1];
	memcpy (tmp_Mtrl, Mtrl, sizeof(D3DMATERIAL7)*nMtrl);
	memcpy (tmp_Mtrl+nMtrl, &mtrl, sizeof(D3DMATERIAL7));
	if (nMtrl) delete []Mtrl;
	Mtrl = tmp_Mtrl;
	return nMtrl++;
}

bool Mesh::DeleteMaterial (DWORD matidx)
{
	DWORD i, j;
	if (matidx >= nMtrl) return false;

	// adjust group material indices
	for (i = 0; i < nGrp; i++) {
		if (Grp[i].MtrlIdx >= matidx) {
			if (Grp[i].MtrlIdx == matidx) Grp[i].MtrlIdx = 0;
			else Grp[i].MtrlIdx--;
		}
	}

	// remove material from the list
	D3DMATERIAL7 *tmp_Mtrl = 0;
	if (nMtrl > 1) {
		tmp_Mtrl = new D3DMATERIAL7[nMtrl-1];
		for (i = j = 0; i < nMtrl; i++) {
			if (i != matidx) memcpy (tmp_Mtrl+j++, Mtrl+i, sizeof(D3DMATERIAL7));
		}
	}
	delete []Mtrl;
	Mtrl = tmp_Mtrl;
	nMtrl--;
	return true;
}

void Mesh::Clear ()
{
	for (DWORD i = 0; i < nGrp; i++) {
		delete []Grp[i].Vtx;
		delete []Grp[i].Idx;
	}
	if (nGrp) {
		delete []Grp;
		nGrp = 0;
	}
	if (nMtrl) {
		delete []Mtrl;
		nMtrl = 0;
	}
	if (GrpVis) {
		delete []GrpCnt;
		delete []GrpRad;
		delete []GrpVis;
		GrpVis = 0;
	}
	GrpSetup = false;
}

void Mesh::ScaleGroup (DWORD grp, D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
	int i, nv = Grp[grp].nVtx;
	NTVERTEX *vtx = Grp[grp].Vtx;
	for (i = 0; i < nv; i++) {
		vtx[i].x *= sx;
		vtx[i].y *= sy;
		vtx[i].z *= sz;
	}
	if (sx == sy && sx == sz) return; // no change in normals
	D3DVALUE snx = sy*sz, sny = sx*sz, snz = sx*sy;
	for (i = 0; i < nv; i++) {
		vtx[i].nx *= snx;
		vtx[i].ny *= sny;
		vtx[i].nz *= snz;
		D3DVALUE ilen = (D3DVALUE)(1.0/sqrt (vtx[i].nx*vtx[i].nx + vtx[i].ny*vtx[i].ny + vtx[i].nz*vtx[i].nz));
		vtx[i].nx *= ilen;
		vtx[i].ny *= ilen;
		vtx[i].nz *= ilen;
	}
	if (GrpSetup) SetupGroup (grp);
}

void Mesh::Scale (D3DVALUE sx, D3DVALUE sy, D3DVALUE sz)
{
	for (DWORD grp = 0; grp < nGrp; grp++)
		ScaleGroup (grp, sx, sy, sz);
}

void Mesh::TranslateGroup (DWORD grp, D3DVALUE dx, D3DVALUE dy, D3DVALUE dz)
{
	int i, nv = Grp[grp].nVtx;
	NTVERTEX *vtx = Grp[grp].Vtx;
	for (i = 0; i < nv; i++) {
		vtx[i].x += dx;
		vtx[i].y += dy;
		vtx[i].z += dz;
	}
	if (GrpSetup) {
		GrpCnt[grp].x += dx;
		GrpCnt[grp].y += dy;
		GrpCnt[grp].z += dz;
	}
}

void Mesh::Translate (D3DVALUE dx, D3DVALUE dy, D3DVALUE dz)
{
	for (DWORD grp = 0; grp < nGrp; grp++)
		TranslateGroup (grp, dx, dy, dz);
}

void Mesh::RotateGroup (DWORD grp, RotAxis axis, D3DVALUE angle)
{
	int i, nv = Grp[grp].nVtx;
	NTVERTEX *vtx = Grp[grp].Vtx;
	D3DVALUE cosa = (D3DVALUE)cos(angle), sina = (D3DVALUE)sin(angle);
	switch (axis) {
	case ROTATE_X:
		for (i = 0; i < nv; i++) {
			D3DVALUE y = vtx[i].y, z = vtx[i].z;
			vtx[i].y = cosa*y - sina*z;
			vtx[i].z = sina*y + cosa*z;
			D3DVALUE ny = vtx[i].ny, nz = vtx[i].nz;
			vtx[i].ny = cosa*ny - sina*nz;
			vtx[i].nz = sina*ny + cosa*nz;
		}
		if (GrpSetup) {
			D3DVALUE y = GrpCnt[grp].y, z = GrpCnt[grp].z;
			GrpCnt[grp].y = cosa*y - sina*z;
			GrpCnt[grp].z = sina*y + cosa*z;
		}
		break;
	case ROTATE_Y:
		for (i = 0; i < nv; i++) {
			D3DVALUE x = vtx[i].x, z = vtx[i].z;
			vtx[i].x = cosa*x - sina*z;
			vtx[i].z = sina*x + cosa*z;
			D3DVALUE nx = vtx[i].nx, nz = vtx[i].nz;
			vtx[i].nx = cosa*nx - sina*nz;
			vtx[i].nz = sina*nx + cosa*nz;
		}
		if (GrpSetup) {
			D3DVALUE x = GrpCnt[grp].x, z = GrpCnt[grp].z;
			GrpCnt[grp].x = cosa*x - sina*z;
			GrpCnt[grp].z = sina*x + cosa*z;
		}
		break;
	case ROTATE_Z:
		for (i = 0; i < nv; i++) {
			D3DVALUE x = vtx[i].x, y = vtx[i].y;
			vtx[i].x = cosa*x - sina*y;
			vtx[i].y = sina*x + cosa*y;
			D3DVALUE nx = vtx[i].nx, ny = vtx[i].ny;
			vtx[i].nx = cosa*nx - sina*ny;
			vtx[i].ny = sina*nx + cosa*ny;
		}
		if (GrpSetup) {
			D3DVALUE x = GrpCnt[grp].x, y = GrpCnt[grp].y;
			GrpCnt[grp].x = cosa*x - sina*y;
			GrpCnt[grp].y = sina*x + cosa*y;
		}
		break;
	}
}

void Mesh::Rotate (RotAxis axis, D3DVALUE angle)
{
	for (DWORD grp = 0; grp < nGrp; grp++)
		RotateGroup (grp, axis, angle);
}

void Mesh::TransformGroup (DWORD grp, const D3DMATRIX &mat)
{
	int i, nv = Grp[grp].nVtx;
	NTVERTEX *vtx = Grp[grp].Vtx;
	FLOAT x, y, z, w;

	for (i = 0; i < nv; i++) {
		NTVERTEX &v = vtx[i];
		x = v.x*mat._11 + v.y*mat._21 + v.z* mat._31 + mat._41;
		y = v.x*mat._12 + v.y*mat._22 + v.z* mat._32 + mat._42;
		z = v.x*mat._13 + v.y*mat._23 + v.z* mat._33 + mat._43;
		w = v.x*mat._14 + v.y*mat._24 + v.z* mat._34 + mat._44;
    	v.x = x/w;
		v.y = y/w;
		v.z = z/w;

		x = v.nx*mat._11 + v.ny*mat._21 + v.nz* mat._31;
		y = v.nx*mat._12 + v.ny*mat._22 + v.nz* mat._32;
		z = v.nx*mat._13 + v.ny*mat._23 + v.nz* mat._33;
		w = 1.0f/(FLOAT)sqrt (x*x + y*y + z*z);
		v.nx = x*w;
		v.ny = y*w;
		v.nz = z*w;
	}
	if (GrpSetup) SetupGroup (grp);
}

void Mesh::Transform (const D3DMATRIX &mat)
{
	for (DWORD grp = 0; grp < nGrp; grp++)
		TransformGroup (grp, mat);
}

void Mesh::TexScaleGroup (DWORD grp, D3DVALUE su, D3DVALUE sv)
{
	int i, nv = Grp[grp].nVtx;
	NTVERTEX *vtx = Grp[grp].Vtx;
	for (i = 0; i < nv; i++) {
		vtx[i].tu *= su;
		vtx[i].tv *= sv;
	}
}

void Mesh::TexScale (D3DVALUE su, D3DVALUE sv)
{
	for (DWORD grp = 0; grp < nGrp; grp++)
		TexScaleGroup (grp, su, sv);
}

void Mesh::CalcNormals (DWORD grp, bool missingonly)
{
	const float eps = 1e-8f;
	int i, nv = Grp[grp].nVtx, nt = Grp[grp].nIdx/3;
	WORD *idx = Grp[grp].Idx;
	NTVERTEX *vtx = Grp[grp].Vtx;
	bool *calcNml = new bool[nv];
	if (missingonly) {
		for (i = 0; i < nv; i++) {
			if (vtx[i].nx*vtx[i].nx + vtx[i].ny*vtx[i].ny + vtx[i].nz*vtx[i].nz > 0.1f) {
				calcNml[i] = false; // flag for "leave normal alone"
			} else {
				calcNml[i] = true;
				vtx[i].nx = vtx[i].ny = vtx[i].nz = 0.0f;
			}
		}
	} else {
		for (i = 0; i < nv; i++) {
			calcNml[i] = true;
			vtx[i].nx = vtx[i].ny = vtx[i].nz = 0.0f;
		}
	}
	for (i = 0; i < nt; i++) {
		DWORD i0 = idx[i*3], i1 = idx[i*3+1], i2 = idx[i*3+2];
		if (!calcNml[i0] && !calcNml[i1] && !calcNml[i2])
			continue; // nothing to do for this triangle
		D3DVECTOR V01 = { vtx[i1].x - vtx[i0].x, vtx[i1].y - vtx[i0].y, vtx[i1].z - vtx[i0].z };
		D3DVECTOR V02 = { vtx[i2].x - vtx[i0].x, vtx[i2].y - vtx[i0].y, vtx[i2].z - vtx[i0].z };
		D3DVECTOR V12 = { vtx[i2].x - vtx[i1].x, vtx[i2].y - vtx[i1].y, vtx[i2].z - vtx[i1].z };
		D3DVECTOR nm = D3DMath_CrossProduct (V01, V02);
		D3DVALUE len = D3DMath_Length (nm);

		if (len >= eps) {
			nm.x /= len, nm.y /= len, nm.z /= len;
			D3DVALUE d01 = D3DMath_Length(V01);
			D3DVALUE d02 = D3DMath_Length(V02);
			D3DVALUE d12 = D3DMath_Length(V12);
			if (calcNml[i0]) {
				D3DVALUE a0 = acos((d01 * d01 + d02 * d02 - d12 * d12) / (2.0f * d01 * d02));
				vtx[i0].nx += nm.x * a0, vtx[i0].ny += nm.y * a0, vtx[i0].nz += nm.z * a0;
			}
			if (calcNml[i1]) {
				D3DVALUE a1 = acos((d01 * d01 + d12 * d12 - d02 * d02) / (2.0f * d01 * d12));
				vtx[i1].nx += nm.x * a1, vtx[i1].ny += nm.y * a1, vtx[i1].nz += nm.z * a1;
			}
			if (calcNml[i2]) {
				D3DVALUE a2 = acos((d02 * d02 + d12 * d12 - d01 * d01) / (2.0f * d02 * d12));
				vtx[i2].nx += nm.x * a2, vtx[i2].ny += nm.y * a2, vtx[i2].nz += nm.z * a2;
			}
		}
	}
	for (i = 0; i < nv; i++)
		if (calcNml[i]) {
			D3DVECTOR nm = { vtx[i].nx, vtx[i].ny, vtx[i].nz };
			D3DVALUE len = D3DMath_Length(nm);
			vtx[i].nx /= len, vtx[i].ny /= len, vtx[i].nz /= len;
		}
	delete []calcNml;
}

void Mesh::CalcTexCoords (DWORD grp)
{
	// quick hack. not globally usable

	int i, nv = Grp[grp].nVtx;
	NTVERTEX *vtx = Grp[grp].Vtx;
	double ipi = 1.0/Pi, i2pi = 0.5/Pi;

	for (i = 0; i < nv; i++) {
		D3DVECTOR pos = {vtx[i].x, vtx[i].y, vtx[i].z};
		D3DMath_Normalise (pos);
		double tht = acos (pos.y);
		double phi = atan2 (pos.z, pos.x);
		vtx[i].tu = (D3DVALUE)(phi >= 0.0 ? phi*i2pi : (phi+Pi2)*i2pi);
		vtx[i].tv = (D3DVALUE)(tht*ipi);
	}
}

void Mesh::SetTexMixture (DWORD grp, DWORD ntex, float mix)
{
}

void Mesh::SetTexMixture (DWORD ntex, float mix)
{
}

void Mesh::GlobalEnableSpecular (bool enable)
{
	bEnableSpecular = enable;
}

void Mesh::EnableMatAlpha (bool enable)
{
	bModulateMatAlpha = enable;
}

istream &operator>> (istream &is, Mesh &mesh)
{
	char cbuf[256];
	int i, j, g, ngrp, nvtx, ntri, nidx, nmtrl, mtrl_idx, ntex, tex_idx, flag, res;
	DWORD uflag;
	WORD zbias;
	D3DMATERIAL7 mtrl;
	bool term, staticmesh = false;

	mesh.Clear();

	if (!is.getline (cbuf, 256)) return is;
	if (strcmp (cbuf, "MSHX1")) return is;

	for (;;) {
		if (!is.getline (cbuf, 256)) return is;
		if (!_strnicmp (cbuf, "GROUPS", 6)) {
			if (sscanf (cbuf+6, "%d", &ngrp) != 1) return is;
			break;
		} else if (!_strnicmp (cbuf, "STATICMESH", 10)) {
			staticmesh = true;
		}
	}

	mesh.Labels = new Mesh::tex_file[ngrp];
	int zeroSize = 256 * ngrp;
	ZeroMemory(mesh.Labels, zeroSize);
	for (g = 0, term = false; g < ngrp && !term; g++) {

		// set defaults
		NTVERTEX *vtx;
		WORD *idx;
		mtrl_idx = SPEC_INHERIT;
		tex_idx  = SPEC_INHERIT;
		zbias    = 0;
		flag     = (staticmesh ? 0x04 : 0);
		uflag    = 0;
		bool bnormal = true, calcnml = false;
		bool flipidx = false;
		nvtx = ntri = 0;

		for (;;) {
			if (!is.getline (cbuf, 256)) { term = true; break; }
			if (!_strnicmp (cbuf, "MATERIAL", 8)) {       // read material index
				sscanf (cbuf+8, "%d", &mtrl_idx);
				mtrl_idx--;
			} else if (!_strnicmp (cbuf, "TEXTURE", 7)) { // read texture index
				sscanf (cbuf+7, "%d", &tex_idx);
				tex_idx--;
			} else if (!_strnicmp (cbuf, "ZBIAS", 5)) {   // read z-bias
				sscanf (cbuf+5, "%hu", &zbias);
			} else if (!_strnicmp (cbuf, "TEXWRAP", 7)) { // read wrap flags
				char uvstr[10] = "";
				sscanf (cbuf+7, "%9s", uvstr);
				if (uvstr[0] == 'U' || uvstr[1] == 'U') flag |= 0x01;
				if (uvstr[0] == 'V' || uvstr[1] == 'V') flag |= 0x02;
			} else if (!_strnicmp (cbuf, "NONORMAL", 8)) {
				bnormal = false; calcnml = true;
			} else if (!_strnicmp (cbuf, "FLAG", 4)) {
				sscanf (cbuf+4, "%lx", &uflag);
			} else if (!_strnicmp (cbuf, "FLIP", 4)) {
				flipidx = true;
			} else if (!_strnicmp (cbuf, "LABEL", 5)) {
				sscanf(cbuf + 5, "%255s", mesh.Labels[g].File);
			} else if (!_strnicmp (cbuf, "STATIC", 6)) {
				flag |= 0x04;
			} else if (!_strnicmp (cbuf, "DYNAMIC", 7)) {
				flag ^= 0x04;
			} else if (!_strnicmp (cbuf, "GEOM", 4)) {    // read geometry
				if (sscanf (cbuf+4, "%d%d", &nvtx, &ntri) != 2) break; // parse error - skip group
				nidx = ntri*3;
				vtx = new NTVERTEX[nvtx];
				ZeroMemory (vtx, sizeof (NTVERTEX)*nvtx);
				for (i = 0; i < nvtx; i++) {
					NTVERTEX &v = vtx[i];
					if (!is.getline (cbuf, 256)) {
						delete []vtx;
						nvtx = 0;
						break;
					}
					if (bnormal) {
						j = sscanf (cbuf, "%f%f%f%f%f%f%f%f",
							&v.x, &v.y, &v.z, &v.nx, &v.ny, &v.nz, &v.tu, &v.tv);
						if (j < 6) calcnml = true;
					} else {
						j = sscanf (cbuf, "%f%f%f%f%f",
							&v.x, &v.y, &v.z, &v.tu, &v.tv);
					}
				}
				idx = new WORD[nidx];
				ZeroMemory (idx, sizeof (WORD)*nidx);
				for (i = j = 0; i < ntri; i++) {
					if (!is.getline (cbuf, 256)) {
						delete []vtx;
						delete []idx;
						nvtx = nidx = 0;
						break;
					}
					sscanf (cbuf, "%hd%hd%hd", idx+j, idx+j+1, idx+j+2);
					j += 3;
				}
				if (flipidx)
					for (i = 0; i < ntri; i++) {
						WORD tmp = idx[i*3+1]; idx[i*3+1] = idx[i*3+2]; idx[i*3+2] = tmp;
					}

				break;
			}
		}
		if (nvtx && nidx) {
			mesh.AddGroup (vtx, nvtx, idx, nidx, mtrl_idx, tex_idx, zbias);
			mesh.Grp[g].Flags = flag;
			mesh.Grp[g].UsrFlag = uflag;
			if (calcnml) mesh.CalcNormals (g, true);
			if (flag & 0x04) mesh.MakeGroupVertexBuffer (g);
		}
	}

	// read material list
	if (is.getline (cbuf, 256) && !strncmp (cbuf, "MATERIALS", 9) && (sscanf (cbuf+9, "%d", &nmtrl) == 1)) {
		Str256 *matname = new Str256[nmtrl];
		mesh.MatNames = new Mesh::tex_file[nmtrl];
		Str256 mnm;
		for (i = 0; i < nmtrl; i++) {
			Str256 *curr = &matname[i];
			is.getline (cbuf, 256);
			sscanf (cbuf, "%s", matname[i]);
			ZeroMemory(&mesh.MatNames[i], 256);
			int len = strlen(matname[i]);
			for (int c = 0; c < 255; c++)
			{
				mesh.MatNames[i].File[c] = cbuf[c];
			}

		}
		for (i = 0; i < nmtrl; i++) {
			ZeroMemory (&mtrl, sizeof (D3DMATERIAL7));
			is.getline (cbuf, 256);
			sscanf (cbuf+8, "%255s", mnm);
			is.getline (cbuf, 256);
			sscanf (cbuf, "%f%f%f%f", &mtrl.diffuse.r, &mtrl.diffuse.g, &mtrl.diffuse.b, &mtrl.diffuse.a);
			is.getline (cbuf, 256);
			sscanf (cbuf, "%f%f%f%f", &mtrl.ambient.r, &mtrl.ambient.g, &mtrl.ambient.b, &mtrl.ambient.a);
			is.getline (cbuf, 256);
			res = sscanf (cbuf, "%f%f%f%f%f", &mtrl.specular.r, &mtrl.specular.g, &mtrl.specular.b, &mtrl.specular.a, &mtrl.power);
			if (res < 5) mtrl.power = 0.0;
			is.getline (cbuf, 256);
			sscanf (cbuf, "%f%f%f%f", &mtrl.emissive.r, &mtrl.emissive.g, &mtrl.emissive.b, &mtrl.emissive.a);
			mesh.AddMaterial (mtrl);
		}
		delete []matname;
	}

	// read texture list
	if (is.getline (cbuf, 256) && !strncmp (cbuf, "TEXTURES", 8) && (sscanf (cbuf+8, "%d", &ntex) == 1)) {
		Str256 texname, flagstr;
		mesh.nTex = ntex;
		mesh.TexFiles = new Mesh::tex_file[ntex];
		for (i = 0; i < ntex; i++) {
			is.getline (cbuf, 256);
			flagstr[0] = '\0';
			sscanf (cbuf, "%255s%255s", texname, flagstr);
			if (texname[0] != '0' || texname[1] != '\0') {
				bool uncompress = (toupper(flagstr[0]) == 'D');
			}
			ZeroMemory(&mesh.TexFiles[i], 256);
			int length = strlen(texname);
			for (int t = 0; t < length; t++) mesh.TexFiles[i].File[t] = texname[t];
		}
	}

	mesh.Setup();
	is.clear();
	return is;
}

bool Mesh::bEnableSpecular = false;

// =======================================================================
// Class MeshManager

// =======================================================================
// Nonmember functions


// =======================================================================
// Create a sphere patch.
// nlng is the number of patches required to span the full 360� in longitude
// nlat is the number of patches required to span the latitude range from 0 to 90�
// 0 <= ilat < nlat is the actual latitude strip the patch is to cover
// res >= 1 is the resolution of the patch (= number of internal latitude strips in the patch)
// bseg, if given, is the number of of polygon segments on the lower base line of the patch.
// Default is (nlat-ilat)*res. bseg is ignored for triangular patches (i.e. where upper
// latitude is 90�)

void CreateSpherePatch (Mesh &mesh, int nlng, int nlat, int ilat, int res, int bseg, bool reduce, bool outside)
{
	const float c1 = 1.0f, c2 = 0.0f; // -1.0f/512.0f; // assumes 256x256 texture patches
	//const float c1 = 1.0f-1.0f/256.0f, c2 = 1.0f/512.f;
	int i, j, nVtx, nIdx, nseg, n, nofs0, nofs1;
	double minlat, maxlat, lat, minlng, maxlng, lng;
	double slat, clat, slng, clng;
	WORD tmp;
	
	minlat = Pi05 * (double)ilat/(double)nlat;
	maxlat = Pi05 * (double)(ilat+1)/(double)nlat;
	minlng = 0;
	maxlng = Pi2/(double)nlng;
	if (bseg < 0 || ilat == nlat-1) bseg = (nlat-ilat)*res;

	// generate nodes
	nVtx = (bseg+1)*(res+1);
	if (reduce) nVtx -= ((res+1)*res)/2;
	NTVERTEX *Vtx = new NTVERTEX[nVtx];

	for (i = n = 0; i <= res; i++) {  // loop over longitudinal strips
		lat = minlat + (maxlat-minlat) * (double)i/(double)res;
		slat = sin(lat), clat = cos(lat);
		nseg = (reduce ? bseg-i : bseg);
		for (j = 0; j <= nseg; j++) {
			lng = (nseg ? minlng + (maxlng-minlng) * (double)j/(double)nseg : 0.0);
			slng = sin(lng), clng = cos(lng);
			Vtx[n].x = Vtx[n].nx = D3DVAL(clat*clng);
			Vtx[n].y = Vtx[n].ny = D3DVAL(slat);
			Vtx[n].z = Vtx[n].nz = D3DVAL(clat*slng);
			Vtx[n].tu = D3DVAL(nseg ? (c1*j)/nseg+c2 : 0.5); // overlap to avoid seams
			Vtx[n].tv = D3DVAL((c1*(res-i))/res+c2);

			if (!outside) {
				Vtx[n].nx = - Vtx[n].nx;
				Vtx[n].ny = - Vtx[n].ny;
				Vtx[n].nz = - Vtx[n].nz;
			}

			n++;
		}
	}

	// generate faces
	nIdx = (reduce ? res * (2*bseg-res) : 2*res*bseg) * 3;
	WORD *Idx = new WORD[nIdx];

	for (i = n = nofs0 = 0; i < res; i++) {
		nseg = (reduce ? bseg-i : bseg);
		nofs1 = nofs0+nseg+1;
		for (j = 0; j < nseg; j++) {
			Idx[n++] = nofs0+j;
			Idx[n++] = nofs1+j;
			Idx[n++] = nofs0+j+1;
			if (reduce && j == nseg-1) break;
			Idx[n++] = nofs0+j+1;
			Idx[n++] = nofs1+j;
			Idx[n++] = nofs1+j+1;
		}
		nofs0 = nofs1;
	}
	if (!outside)
		for (i = 0; i < nIdx/3; i += 3)
			tmp = Idx[i+1], Idx[i+1] = Idx[i+2], Idx[i+2] = tmp;

	mesh.Clear();
	mesh.AddGroup (Vtx, nVtx, Idx, nIdx, SPEC_INHERIT, SPEC_INHERIT);
	mesh.Setup ();
}
