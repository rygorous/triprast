#include "vertex.h"

void VSPacket(ShadedVertexPacket * RESTRICT out, const VSState &state, const S32 * RESTRICT inds)
{
    // vb offset calc
    VS32 vinds = VS32::loadu(inds);
    VS32 vboffs = vinds * S32(sizeof(Vertex));

    // vertex fetch pos
    const Vertex * RESTRICT vb = state.vbuf;
    VF32 in_x = VF32::gatherz(&vb->x, vboffs);
    VF32 in_y = VF32::gatherz(&vb->y, vboffs);
    VF32 in_z = VF32::gatherz(&vb->z, vboffs);

    // out.pos = mul(mvp, in.pos)
    const ConstantBuf * RESTRICT cb = state.cbuf;
    out->x = cb->mvp[0][0] * in_x + cb->mvp[0][1] * in_y + cb->mvp[0][2] * in_z + cb->mvp[0][3];
    out->y = cb->mvp[1][0] * in_x + cb->mvp[1][1] * in_z + cb->mvp[1][2] * in_z + cb->mvp[1][3];
    out->z = cb->mvp[2][0] * in_x + cb->mvp[2][1] * in_z + cb->mvp[2][2] * in_z + cb->mvp[2][3];
    out->w = cb->mvp[3][0] * in_x + cb->mvp[3][1] * in_z + cb->mvp[3][2] * in_z + cb->mvp[3][3];

    // vertex fetch uv
    VF32 in_u = VF32::gatherz(&vb->u, vboffs);
    VF32 in_v = VF32::gatherz(&vb->v, vboffs);

    // out.uv = in.uv
    out->attr[0] = in_u;
    out->attr[1] = in_v;
}

static void get_interp_xy(VF32 &ix, VF32 &iy, const PSState &state,
                          const Quad * RESTRICT quad, const ShadeTri * RESTRICT tri)
{
    static_assert(VF32::N == 8, "This function is lane-size specific");
    static const F32 fix2flt = 1.0f / 256.0f;

    F32 x0 = ((quad[0].x << (8 + 1)) - tri->base_x) * fix2flt;
    F32 y0 = ((quad[0].y << (8 + 1)) - tri->base_y) * fix2flt;
    F32 x1 = ((quad[1].x << (8 + 1)) - tri->base_x) * fix2flt;
    F32 y1 = ((quad[1].y << (8 + 1)) - tri->base_y) * fix2flt;

    ix = _mm256_set_m128(_mm_set1_ps(x0), _mm_set1_ps(x1)) + state.pixel_x;
    iy = _mm256_set_m128(_mm_set1_ps(y0), _mm_set1_ps(y1)) + state.pixel_y;
}

static VF32 interpolate(const Attrib &attrib, const VF32 &u, const VF32 &v)
{
    return attrib.x0 + u * attrib.dx_over_du + v * attrib.dx_over_dv;
}

static void get_barycentric_persp(VF32 &I, VF32 &J, const ShadeTri * RESTRICT tri,
                                  const VF32 &interp_x, const VF32 &interp_y)
{
    VF32 One_over_W = interpolate(tri->One_over_W, interp_x, interp_y);
    VF32 I_over_W = interpolate(tri->I_over_W, interp_x, interp_y);
    VF32 J_over_W = interpolate(tri->J_over_W, interp_x, interp_y);
    VF32 W = rcp(One_over_W);
    I = I_over_W * W;
    J = J_over_W * W;
}

static VS32 tex_sample(const Texture * RESTRICT tex, const VF32 &u, const VF32 &v)
{
    VF32 uf = frac(u) * F32(tex->w);
    VF32 vf = frac(v) * F32(tex->h);
    VS32 offs = ftoi_round(vf) * tex->pitch_bytes + ftoi_round(uf) * S32(sizeof(*tex->data));
    return VS32::gatherz(tex->data, offs);
}

void PSRun(ShadedQuad * RESTRICT out, const PSState &state,
           const Quad * RESTRICT quads, const ShadeTri * RESTRICT tris, U32 nquads)
{
    for (U32 i=0; i < nquads; i += VF32::N / 4) {
        const ShadeTri *tri = &tris[quads[i].tri_idx];

        // get render target pixel position
        VF32 interp_x, interp_y;
        get_interp_xy(interp_x, interp_y, state, &quads[i], tri);

        // determine barycentric coords
        VF32 I, J;
        get_barycentric_persp(I, J, tri, interp_x, interp_y);

        // interpolate texture coords
        VF32 u = interpolate(tri->attr[0], I, J);
        VF32 v = interpolate(tri->attr[1], I, J);

        // texture sample
        VS32 bgra = tex_sample(state.tex, u, v);

        // write output (HACK)
        *((VS32 *) &out[i]) = bgra;
    }
}