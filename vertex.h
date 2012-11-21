#ifndef __VERTEX_H__
#define __VERTEX_H__

#include "types.h"
#include "simd.h"

static const int MAX_ATTRIBS = 4;

struct Vertex {
	F32 x, y, z;
	F32 u, v;
};

struct ConstantBuf {
    F32 mvp[4][4];
};

struct ShadedVertexPacket {
    VF32 x, y, z, w;            // in clip space
    VF32 attr[MAX_ATTRIBS];
};

struct VSState {
    const Vertex *vbuf;
    const ConstantBuf *cbuf;
};

void VSPacket(ShadedVertexPacket * RESTRICT out, const VSState &state, const S32 * RESTRICT inds);

struct Attrib {
    F32 x0, dx_over_du, dx_over_dv;
};

struct ShadeTri {
    Fix24_8 base_x, base_y;     // screen-space position of base vertex
    Attrib Z;
    Attrib I_over_W;
    Attrib J_over_W;
    Attrib One_over_W;

    Attrib attr[MAX_ATTRIBS];
};

struct Quad {
    U16 x, y;                   // integer render target position, in quads
    U16 mask:2;                 // coverage mask
    U16 tri_idx:14;             // index into array of ShadeTri's
};

struct ShadedQuad {
    Pixel pix[4];               // shaded pixel values for blend/resolve
};

struct Texture {
    Pixel *data;
    S32 w, h, pitch_bytes;
};

struct PSState {
    const Texture *tex;

    VF32 pixel_x;               // x position of pixels relative to quad origin
    VF32 pixel_y;               // y position of pixels relative to quad origin
};

#endif