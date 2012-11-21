#ifndef __SIMD_H__
#define __SIMD_H__

#include "types.h"
#include <intrin.h>
#include <immintrin.h>

#pragma warning (push)
#pragma warning (disable: 4201) // nameless struct/union

// ---- ints

typedef U8 VMask;

union VS32 {
    enum { N = 8 };
    S32 lane[N];
    __m256i simd;
    struct {
        __m128i lo, hi;
    };

    VS32() {}
    VS32(S32 s) : lo(_mm_set1_epi32(s)), hi(_mm_set1_epi32(s)) {}
    VS32(__m256i in) : simd(in) {}
    VS32(__m128i lo, __m128i hi) : lo(lo), hi(hi) {}

    static VS32 zero()                      { return VS32(_mm_setzero_si128(), _mm_setzero_si128()); }
    static VS32 load(const S32 *ptr)        { return VS32(_mm_load_si128((__m128i *) (ptr + 0)), _mm_load_si128((__m128i *) (ptr + 4))); }
    static VS32 load1(const S32 *ptr)       { return VS32(_mm_set1_epi32(*ptr), _mm_set1_epi32(*ptr)); }
    static VS32 loadu(const S32 *ptr)       { return VS32(_mm_loadu_si128((__m128i *) (ptr + 0)), _mm_loadu_si128((__m128i *) (ptr + 4))); }
    static VS32 gatherz(const void *base, const VS32 &offs) { VS32 x; x.gather(base, offs); return x; }
    static VS32 gatherz(const void *base, const VS32 &offs, VMask mask) { VS32 x = zero(); x.gather(base, offs, mask); return x; }

    VS32 &operator +=(const VS32 &b)        { lo = _mm_add_epi32(lo, b.lo); hi = _mm_add_epi32(hi, b.hi); return *this; }
    VS32 &operator -=(const VS32 &b)        { lo = _mm_sub_epi32(lo, b.lo); hi = _mm_sub_epi32(hi, b.hi); return *this; }
    VS32 &operator *=(const VS32 &b)        { lo = _mm_mullo_epi32(lo, b.lo); hi = _mm_mullo_epi32(hi, b.hi); return *this; }

    void gather(const void *base, const VS32 &offs);
    void gather(const void *base, const VS32 &offs, VMask mask);
};

inline VS32 operator +(const VS32 &a, const VS32 &b)    { return VS32(_mm_add_epi32(a.lo, b.lo), _mm_add_epi32(a.hi, b.hi)); }
inline VS32 operator *(const VS32 &a, const VS32 &b)    { return VS32(_mm_mullo_epi32(a.lo, b.lo), _mm_mullo_epi32(a.hi, b.hi)); }

inline void VS32::gather(const void *base, const VS32 &offs)
{
    const U8 *base_bytes = (const U8 *) base;
    for (U32 i=0; i < N; i++)
        lane[i] = *((S32 *) (base_bytes + offs.lane[i]));
}

inline void VS32::gather(const void *base, const VS32 &offs, VMask mask)
{
    const U8 *base_bytes = (const U8 *) base;
    unsigned long i, m = mask;
    while (m) {
        _BitScanForward(&i, m);
        m &= m - 1; // remove first set bit
        lane[i] = *((S32 *) (base_bytes + offs.lane[i]));
    }
}

// ---- floats

union VF32 {
    enum { N = 8 };
    F32 lane[N];
    __m256 simd;

    VF32() {}
    VF32(F32 s) : simd(_mm256_set1_ps(s)) {}
    VF32(__m256 in) : simd(in) {}

    static VF32 zero()                      { return _mm256_setzero_ps(); }
    static VF32 load(const F32 *ptr)		{ return _mm256_load_ps(ptr); }
    static VF32 load1(const F32 *ptr)		{ return _mm256_broadcast_ss(ptr); }
    static VF32 loadu(const F32 *ptr)		{ return _mm256_loadu_ps(ptr); }
    static VF32 bits1(const U32 bits)		{ return _mm256_castsi256_ps(_mm256_set1_epi32((S32) bits)); }
    static VF32 gatherz(const void *base, const VS32 &offs) { VF32 x; x.gather(base, offs); return x; }
    static VF32 gatherz(const void *base, const VS32 &offs, VMask mask) { VF32 x = zero(); x.gather(base, offs, mask); return x; }

    VF32 &operator +=(const VF32 &b)	    { simd = _mm256_add_ps(simd, b.simd); return *this; }
    VF32 &operator -=(const VF32 &b)	    { simd = _mm256_sub_ps(simd, b.simd); return *this; }
    VF32 &operator *=(const VF32 &b)	    { simd = _mm256_mul_ps(simd, b.simd); return *this; }
    VF32 &operator /=(const VF32 &b)	    { simd = _mm256_div_ps(simd, b.simd); return *this; }

    void gather(const void *base, const VS32 &offs);
    void gather(const void *base, const VS32 &offs, VMask mask);
};

inline VF32 operator -(const VF32 &a)                   { return _mm256_xor_ps(a.simd, VF32::bits1(0x80000000).simd); }
inline VF32 operator +(const VF32 &a, const VF32 &b)    { return _mm256_add_ps(a.simd, b.simd); }
inline VF32 operator -(const VF32 &a, const VF32 &b)    { return _mm256_sub_ps(a.simd, b.simd); }
inline VF32 operator *(const VF32 &a, const VF32 &b)    { return _mm256_mul_ps(a.simd, b.simd); }
inline VF32 operator /(const VF32 &a, const VF32 &b)    { return _mm256_div_ps(a.simd, b.simd); }

inline VF32 abs(const VF32 &a)                          { return _mm256_and_ps(a.simd, VF32::bits1(0x7fffffff).simd); }
inline VF32 rcp(const VF32 &a)                          { return _mm256_rcp_ps(a.simd); }
inline VF32 rsqrt(const VF32 &a)                        { return _mm256_rsqrt_ps(a.simd); }
inline VF32 sqrt(const VF32 &a)                         { return _mm256_sqrt_ps(a.simd); }
inline VF32 frac(const VF32 &a)                         { return a - _mm256_floor_ps(a.simd); }

inline void VF32::gather(const void *base, const VS32 &offs)
{
    const U8 *base_bytes = (const U8 *) base;
    for (U32 i=0; i < N; i++)
        lane[i] = *((F32 *) (base_bytes + offs.lane[i]));
}

inline void VF32::gather(const void *base, const VS32 &offs, VMask mask)
{
    const U8 *base_bytes = (const U8 *) base;
    unsigned long i, m = mask;
    while (m) {
        _BitScanForward(&i, m);
        m &= m - 1; // remove first set bit
        lane[i] = *((F32 *) (base_bytes + offs.lane[i]));
    }
}

// ---- conversions

inline VS32 float2bits(const VF32 &a)                   { return _mm256_castps_si256(a.simd); }
inline VF32 bits2float(const VS32 &a)                   { return _mm256_castsi256_ps(a.simd); }

inline VF32 itof(const VS32 &a)                         { return _mm256_cvtepi32_ps(a.simd); }
inline VS32 ftoi_round(const VF32 &a)                   { return _mm256_cvtps_epi32(a.simd); }
inline VS32 ftoi(const VF32 &a)                         { return _mm256_cvttps_epi32(a.simd); }

#pragma warning (pop)

#endif