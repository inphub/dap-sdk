/*
Implementation by Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <stdio.h>
#include <string.h>
#include <smmintrin.h>
#include <wmmintrin.h>
#include <immintrin.h>
#include <emmintrin.h>
#include "align.h"
#include "brg_endian.h"
#include "Xoodoo.h"
#include "Xoodoo-times16-SnP.h"

#if (PLATFORM_BYTE_ORDER != IS_LITTLE_ENDIAN)
#error Expecting a little-endian platform
#endif

/* #define SIMULATE_AVX512 */

#define VERBOSE         0

#if defined(SIMULATE_AVX512)

typedef struct
{
    uint32_t x[16];
} __m512i;

static void _mm512_mask_store_epi64(void *mem_addr, uint8_t k, __m512i a)
{
    uint64_t *p64 = (uint64_t *)mem_addr;
    unsigned int i;

    for ( i = 0; i < 8; ++i ) {
        if ((k & (1 << i)) != 0)
            p64[i] = (uint64_t)a.x[2*i] | ((uint64_t)a.x[2*i+1] << 32);
    }
}

static __m512i _mm512_maskz_load_epi64(uint8_t k, const void *mem_addr)
{
    __m512i r;
    const uint64_t *p64 = (const uint64_t *)mem_addr;
    unsigned int i;

    for ( i = 0; i < 8; ++i ) {
        if ((k & (1 << i)) != 0) {
            r.x[2*i]   = (uint32_t)p64[i];
            r.x[2*i+1] = (uint32_t)(p64[i] >> 32);
        }
        else {
            r.x[2*i]   = 0;
            r.x[2*i+1] = 0;
        }
    }
    return(r);
}

static void _mm512_storeu_si512(__m512i * mem_addr, __m512i a)
{
    uint32_t *p32 = (uint32_t *)mem_addr;
    unsigned int i;

    for ( i = 0; i < 16; ++i )
        p32[i] = a.x[i];
}

#define _mm512_store_si512    _mm512_storeu_si512

static __m512i _mm512_loadu_si512(const __m512i * mem_addr)
{
    __m512i r;
    const uint32_t *p32 = (const uint32_t *)mem_addr;
    unsigned int i;

    for ( i = 0; i < 16; ++i )
        r.x[i] = p32[i];
    return(r);
}

#define _mm512_load_si512    _mm512_loadu_si512

static __m512i _mm512_setzero_si512(void)
{
    __m512i r;
    unsigned int i;

    for ( i = 0; i < 16; ++i )
        r.x[i] = 0;
    return(r);
}

static __m512i _mm512_xor_si512( __m512i a, __m512i b)
{
    __m512i r;
    unsigned int i;

    for ( i = 0; i < 16; ++i )
        r.x[i] = a.x[i] ^ b.x[i];
    return(r);
}

static __m512i _mm512_and_si512( __m512i a, __m512i b)
{
    __m512i r;
    unsigned int i;

    for ( i = 0; i < 16; ++i )
        r.x[i] = a.x[i] & b.x[i];
    return(r);
}

static __m512i _mm512_ternarylogic_epi32(__m512i a, __m512i b, __m512i c, int imm)
{

    if (imm == 0x96)
        return ( _mm512_xor_si512( _mm512_xor_si512( a, b ), c ) );
    if (imm == 0xD2) {
        __m512i t;
        unsigned int i;

        for ( i = 0; i < 16; ++i )
            t.x[i] = ~b.x[i] & c.x[i];
        return ( _mm512_xor_si512( a, t ) );
    }
    printf( "_mm512_ternarylogic_epi64( a, b, c, %02X) not implemented!\n", imm );
    exit(1);

}

static __m512i _mm512_rol_epi32(__m512i a, int offset)
{
    __m512i r;
    unsigned int i;

    for ( i = 0; i < 16; ++i )
        r.x[i] = (a.x[i] << offset) | (a.x[i] >> (32-offset));
    return(r);
}

static __m512i _mm512_slli_epi32(__m512i a, int offset)
{
    __m512i r;
    unsigned int i;

    for ( i = 0; i < 16; ++i )
        r.x[i] = (a.x[i] << offset);
    return(r);
}

static __m512i _mm512_set1_epi32(uint32_t a)
{
    unsigned int i;
    __m512i r;

    for ( i = 0; i < 16; ++i )
        r.x[i] = a;
    return(r);
}

static __m512i _mm512_i32gather_epi32(__m512i idx, const void *p, int scale)
{
    __m512i r;
    unsigned int i;
    for ( i = 0; i < 16; ++i )
        r.x[i] = *(const uint32_t*)((const char*)p + idx.x[i] * scale);
    return(r);
}

static void _mm512_i32scatter_epi32( void *p, __m512i idx, __m512i value, int scale)
{
    unsigned int i;
    for ( i = 0; i < 16; ++i )
        *(uint32_t*)((char*)p + idx.x[i] * scale) = value.x[i];
}

static void _mm512_mask_i32scatter_epi32( void *p, uint16_t k, __m512i idx, __m512i value, int scale)
{
    unsigned int i;
    for ( i = 0; i < 16; ++i ) {
        if ((k & (1 << i)) != 0)
            *(uint32_t*)((char*)p + idx.x[i] * scale) = value.x[i];
    }
}

static __m512i _mm512_setr_epi32(    int e15, int e14, int e13, int e12, int e11, int e10, int e9, int e8, 
                                    int e7,  int e6,  int e5,  int e4,  int e3,  int e2,  int ee1, int ee0) 
{
    __m512i r;

    r.x[ 0] = e15;
    r.x[ 1] = e14;
    r.x[ 2] = e13;
    r.x[ 3] = e12;
    r.x[ 4] = e11;
    r.x[ 5] = e10;
    r.x[ 6] = e9;
    r.x[ 7] = e8;
    r.x[ 8] = e7;
    r.x[ 9] = e6;
    r.x[10] = e5;
    r.x[11] = e4;
    r.x[12] = e3;
    r.x[13] = e2;
    r.x[14] = ee1;
    r.x[15] = ee0;
    return(r);
}

static __m512i _mm512_permutex2var_epi32(__m512i a, __m512i idx, __m512i b)
{
    __m512i r;
    unsigned int i;
    for ( i = 0; i < 16; ++i )
        r.x[i] = (idx.x[i] & 0x10) ? b.x[idx.x[i] & 0x0F] :  a.x[idx.x[i] & 0x0F];
    return(r);
}

static __m512i _mm512_permutexvar_epi32(__m512i idx, __m512i a)
{
    __m512i r;
    unsigned int i;
    for ( i = 0; i < 16; ++i )
        r.x[i] = a.x[idx.x[i]];
    return(r);
}

#endif

typedef __m128i V128;
typedef __m256i V256;
typedef __m512i V512;

#define SnP_laneLengthInBytes   4
#define laneIndex(instanceIndex, lanePosition) ((lanePosition)*16 + instanceIndex)

#define Chi(a,b,c)                  _mm512_ternarylogic_epi32(a,b,c,0xD2)

#define CONST16_32(a)               _mm512_set1_epi32(a)
#define LOAD256u(a)                 _mm256_loadu_si256((const V256 *)&(a))

#define LOAD512(a)                  _mm512_load_si512((const V512 *)&(a))
#define LOAD512u(a)                 _mm512_loadu_si512((const V512 *)&(a))

#define LOAD_GATHER16_32(idx,p)     _mm512_i32gather_epi32(idx, (const void*)(p), 4)
#define STORE_SCATTER16_32(idx,a,p) _mm512_i32scatter_epi32((void*)(p), idx, a, 4)

#define SHUFFLE_LANES_RIGHT(idx, a)  _mm512_permutexvar_epi32(idx, a)

#define ROL32(a, o)                 _mm512_rol_epi32(a, o)
#define SHL32(a, o)                 _mm512_slli_epi32(a, o)

#define SET16_32                    _mm512_setr_epi32

#define STORE128(a, b)              _mm_store_si128((V128 *)&(a), b)
#define STORE128u(a, b)             _mm_storeu_si128((V128 *)&(a), b)
#define STORE256u(a, b)             _mm256_storeu_si256((V256 *)&(a), b)
#define STORE512(a, b)              _mm512_store_si512((V512 *)&(a), b)
#define STORE512u(a, b)             _mm512_storeu_si512((V512 *)&(a), b)

#define AND(a, b)                   _mm512_and_si512(a, b)
#define XOR(a, b)                   _mm512_xor_si512(a, b)
#define XOR3(a,b,c)                 _mm512_ternarylogic_epi32(a,b,c,0x96)
#define XOR256(a, b)                _mm256_xor_si256(a, b)
#define XOR128(a, b)                _mm_xor_si128(a, b)


#define _mm256_storeu2_m128i(hi, lo, a)    _mm_storeu_si128((V128*)(lo), _mm256_castsi256_si128(a)), _mm_storeu_si128((V128*)(hi), _mm256_extracti128_si256(a, 1))

#if (VERBOSE > 0)
    #define    DumpOne(__b,__v,__i) STORE512(__b, __v##__i); \
                                    printf("%02u %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", __i, \
                                      buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15])

    #define    Dump(__t,__v)    {                   \
                            uint32_t    buf[16];    \
                            printf("%s\n", __t);    \
                            DumpOne(buf, __v, 00);  \
                            DumpOne(buf, __v, 01);  \
                            DumpOne(buf, __v, 02);  \
                            DumpOne(buf, __v, 03);  \
                            DumpOne(buf, __v, 10);  \
                            DumpOne(buf, __v, 11);  \
                            DumpOne(buf, __v, 12);  \
                            DumpOne(buf, __v, 13);  \
                            DumpOne(buf, __v, 20);  \
                            DumpOne(buf, __v, 21);  \
                            DumpOne(buf, __v, 22);  \
                            DumpOne(buf, __v, 23);  \
                        }
#else
    #define    Dump(__t,__v)
#endif

#if (VERBOSE >= 1)
    #define    Dump1(__t,__v)    Dump(__t,__v)
#else
    #define    Dump1(__t,__v)
#endif

#if (VERBOSE >= 2)
    #define    Dump2(__t,__v)    Dump(__t,__v)
#else
    #define    Dump2(__t,__v)
#endif

#if (VERBOSE >= 3)
    #define    Dump3(__t,__v)    Dump(__t,__v)
#else
    #define    Dump3(__t,__v)
#endif

#if (VERBOSE > 0)
#define    DUMP32(tt, buf) printf("%s %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", tt, \
                            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15])

#define    DumpLane(__t,__v) {  uint32_t buf[16]; \
                                STORE512(buf[0], __v); \
                                printf("%s %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", __t, \
                                  buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]); }

#else
#define    DUMP32(__t, buf)
#define    DumpLane(__t,__v)
#endif

ALIGN(64) static const uint32_t     oshuffleR_1[]   = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0};
ALIGN(64) static const uint32_t     oshuffleR_2[]   = { 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1};
ALIGN(64) static const uint32_t     oshuffleR_4[]   = { 4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,  0,  1,  2,  3};

ALIGN(64) static const uint32_t     oAllLow12_0[]   = { 0, 1,  2,  3,  4,  5,  6,  7,  8,  9,    10,    11, 16+ 0, 16+ 1, 16+ 2, 16+ 3 };
ALIGN(64) static const uint32_t     oAllLow11_0[]   = { 0, 1,  2,  3,  4,  5,  6,  7,  8,  9,    10, 16+ 0, 16+ 1, 16+ 2, 16+ 3, 16+ 4 };
ALIGN(64) static const uint32_t     oAllLow10_0[]   = { 0, 1,  2,  3,  4,  5,  6,  7,  8,  9, 16+ 0, 16+ 1, 16+ 2, 16+ 3, 16+ 4, 16+ 5 };

ALIGN(64) static const uint32_t     oAllFrom1_7[]   = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16+ 7 };
ALIGN(64) static const uint32_t     oAllFrom1_10[]  = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16+10 };
ALIGN(64) static const uint32_t     oAllFrom1_13[]  = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16+13 };
        
ALIGN(64) static const uint32_t     oAllFrom1_5[]   = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16+ 5 };
ALIGN(64) static const uint32_t     oAllFrom1_8[]   = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16+ 8 };
ALIGN(64) static const uint32_t     oAllFrom1_11[]  = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16+11 };
ALIGN(64) static const uint32_t     oAllFrom1_14[]  = { 1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16+14 };

ALIGN(64) static const uint32_t     oAllFrom3_4[]   = { 3,   4,   5,  6, 7,  8,  9,     10,    11,    12,    13,    14,    15, 16+ 4, 16+ 5, 16+ 6 };
ALIGN(64) static const uint32_t     oAllFrom6_4[]   = { 6,   7,   8,  9, 10, 11, 12,    13,    14,    15, 16+ 4, 16+ 5, 16+ 6, 16+ 7, 16+ 8, 16+ 9};
ALIGN(64) static const uint32_t     oAllFrom9_4[]   = { 9,  10,  11, 12, 13, 14, 15, 16+ 4, 16+ 5, 16+ 6, 16+ 7, 16+ 8, 16+ 9, 16+10, 16+11, 16+12};

ALIGN(64) static const uint32_t     oLow256[]       = {     0,     1,     2,     3,     4,     5,     6,     7, 16+ 0, 16+ 1, 16+ 2, 16+ 3, 16+ 4, 16+ 5, 16+ 6, 16+ 7 };
ALIGN(64) static const uint32_t     oHigh256[]      = {     8,     9,    10,    11,    12,    13,    14,    15, 16+ 8, 16+ 9, 16+10, 16+11, 16+12, 16+13, 16+14, 16+15 };

ALIGN(64) static const uint32_t     oLow128[]       = {     0,     1,     2,     3, 16+ 0, 16+ 1, 16+ 2, 16+ 3,     8,     9,    10,    11, 16+ 8, 16+ 9, 16+10, 16+11 };
ALIGN(64) static const uint32_t     oHigh128[]      = {     4,     5,     6,     7, 16+ 4, 16+ 5, 16+ 6, 16+ 7,    12,    13,    14,    15, 16+12, 16+13, 16+14, 16+15 };

ALIGN(64) static const uint32_t     oLow64[]        = {     0,     1,     4,     5, 16+ 0, 16+ 1, 16+ 4, 16+ 5,     8,     9,    12,    13, 16+ 8, 16+ 9, 16+12, 16+13 };
ALIGN(64) static const uint32_t     oHigh64[]       = {     2,     3,     6,     7, 16+ 2, 16+ 3, 16+ 6, 16+ 7,    10,    11,    14,    15, 16+10, 16+11, 16+14, 16+15 };

ALIGN(64) static const uint32_t     oLow32[]        = {     0,     4,     2,     6,     8,    12,    10,    14, 16+ 0, 16+ 2, 16+ 8, 16+10,     0,     0,     0,     0 };
ALIGN(64) static const uint32_t     oHigh32[]       = {     1,     5,     3,     7,     9,    13,    11,    15, 16+ 1, 16+ 3, 16+ 9, 16+11,     0,     0,     0,     0 };


ALIGN(64) static const uint32_t    oGatherScatterOffsets[] = { 0*12, 1*12, 2*12, 3*12, 4*12, 5*12, 6*12, 7*12, 8*12, 9*12, 10*12, 11*12, 12*12, 13*12, 14*12, 15*12 };
ALIGN(64) static const uint32_t    oGatherScatterOffsetsRoll[] = { 0, 0, 0, 0, 0,  4,  8,  1,  5,  9,  2,  6, 10, 3, 7, 11 }; /* First 4 are dummies */

void Xoodootimes16_InitializeAll(void *states)
{
    memset(states, 0, Xoodootimes16_statesSizeInBytes);
}

void Xoodootimes16_AddBytes(void *states, unsigned int instanceIndex, const unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int sizeLeft = length;
    unsigned int lanePosition = offset/SnP_laneLengthInBytes;
    unsigned int offsetInLane = offset%SnP_laneLengthInBytes;
    const unsigned char *curData = data;
    uint32_t *statesAsLanes = (uint32_t *)states;

    if ((sizeLeft > 0) && (offsetInLane != 0)) {
        unsigned int bytesInLane = SnP_laneLengthInBytes - offsetInLane;
        uint32_t lane = 0;
        if (bytesInLane > sizeLeft)
            bytesInLane = sizeLeft;
        memcpy((unsigned char*)&lane + offsetInLane, curData, bytesInLane);
        statesAsLanes[laneIndex(instanceIndex, lanePosition)] ^= lane;
        sizeLeft -= bytesInLane;
        lanePosition++;
        curData += bytesInLane;
    }

    while(sizeLeft >= SnP_laneLengthInBytes) {
        uint32_t lane = *((const uint32_t*)curData);
        statesAsLanes[laneIndex(instanceIndex, lanePosition)] ^= lane;
        sizeLeft -= SnP_laneLengthInBytes;
        lanePosition++;
        curData += SnP_laneLengthInBytes;
    }

    if (sizeLeft > 0) {
        uint32_t lane = 0;
        memcpy(&lane, curData, sizeLeft);
        statesAsLanes[laneIndex(instanceIndex, lanePosition)] ^= lane;
    }
}

void Xoodootimes16_AddLanesAll(void *states, const unsigned char *data, unsigned int laneCount, unsigned int laneOffset)
{
    V512 *stateAsLanes = (V512 *)states;
    unsigned int i;
    const uint32_t *data32 = (const uint32_t *)data;
    V512 offsets = SET16_32(     0*laneOffset, 1*laneOffset,  2*laneOffset,  3*laneOffset,  4*laneOffset,  5*laneOffset,  6*laneOffset,  7*laneOffset, 
                                8*laneOffset, 9*laneOffset, 10*laneOffset, 11*laneOffset, 12*laneOffset, 13*laneOffset, 14*laneOffset, 15*laneOffset);

    #define Xor_In( argIndex )  stateAsLanes[argIndex] = XOR(stateAsLanes[argIndex], LOAD_GATHER16_32(offsets, &data32[argIndex]))

    if ( laneCount == 12 )  {
        Xor_In( 0 );
        Xor_In( 1 );
        Xor_In( 2 );
        Xor_In( 3 );
        Xor_In( 4 );
        Xor_In( 5 );
        Xor_In( 6 );
        Xor_In( 7 );
        Xor_In( 8 );
        Xor_In( 9 );
        Xor_In( 10 );
        Xor_In( 11 );
    }
    else {
        for(i=0; i<laneCount; i++)
            Xor_In( i );
    }
    #undef  Xor_In
}

void Xoodootimes16_OverwriteBytes(void *states, unsigned int instanceIndex, const unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int sizeLeft = length;
    unsigned int lanePosition = offset/SnP_laneLengthInBytes;
    unsigned int offsetInLane = offset%SnP_laneLengthInBytes;
    const unsigned char *curData = data;
    uint32_t *statesAsLanes = (uint32_t *)states;

    if ((sizeLeft > 0) && (offsetInLane != 0)) {
        unsigned int bytesInLane = SnP_laneLengthInBytes - offsetInLane;
        if (bytesInLane > sizeLeft)
            bytesInLane = sizeLeft;
        memcpy( ((unsigned char *)&statesAsLanes[laneIndex(instanceIndex, lanePosition)]) + offsetInLane, curData, bytesInLane);
        sizeLeft -= bytesInLane;
        lanePosition++;
        curData += bytesInLane;
    }

    while(sizeLeft >= SnP_laneLengthInBytes) {
        uint32_t lane = *((const uint32_t*)curData);
        statesAsLanes[laneIndex(instanceIndex, lanePosition)] = lane;
        sizeLeft -= SnP_laneLengthInBytes;
        lanePosition++;
        curData += SnP_laneLengthInBytes;
    }

    if (sizeLeft > 0) {
        memcpy(&statesAsLanes[laneIndex(instanceIndex, lanePosition)], curData, sizeLeft);
    }
}

void Xoodootimes16_OverwriteLanesAll(void *states, const unsigned char *data, unsigned int laneCount, unsigned int laneOffset)
{
    V512 *stateAsLanes = (V512 *)states;
    unsigned int i;
    const uint32_t *data32 = (const uint32_t *)data;
    V512 offsets = SET16_32(     0*laneOffset, 1*laneOffset,  2*laneOffset,  3*laneOffset,  4*laneOffset,  5*laneOffset,  6*laneOffset,  7*laneOffset, 
                                8*laneOffset, 9*laneOffset, 10*laneOffset, 11*laneOffset, 12*laneOffset, 13*laneOffset, 14*laneOffset, 15*laneOffset);

    #define OverWr( argIndex )  stateAsLanes[argIndex] = LOAD_GATHER16_32(offsets, &data32[argIndex])

    if ( laneCount == 12 )  {
        OverWr( 0 );
        OverWr( 1 );
        OverWr( 2 );
        OverWr( 3 );
        OverWr( 4 );
        OverWr( 5 );
        OverWr( 6 );
        OverWr( 7 );
        OverWr( 8 );
        OverWr( 9 );
        OverWr( 10 );
        OverWr( 11 );
    }
    else {
        for(i=0; i<laneCount; i++)
            OverWr( i );
    }
    #undef  OverWr
}

void Xoodootimes16_OverwriteWithZeroes(void *states, unsigned int instanceIndex, unsigned int byteCount)
{
    unsigned int sizeLeft = byteCount;
    unsigned int lanePosition = 0;
    uint32_t *statesAsLanes = (uint32_t *)states;

    while(sizeLeft >= SnP_laneLengthInBytes) {
        statesAsLanes[laneIndex(instanceIndex, lanePosition)] = 0;
        sizeLeft -= SnP_laneLengthInBytes;
        lanePosition++;
    }

    if (sizeLeft > 0) {
        memset(&statesAsLanes[laneIndex(instanceIndex, lanePosition)], 0, sizeLeft);
    }
}

void Xoodootimes16_ExtractBytes(const void *states, unsigned int instanceIndex, unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int sizeLeft = length;
    unsigned int lanePosition = offset/SnP_laneLengthInBytes;
    unsigned int offsetInLane = offset%SnP_laneLengthInBytes;
    unsigned char *curData = data;
    const uint32_t *statesAsLanes = (const uint32_t *)states;

    if ((sizeLeft > 0) && (offsetInLane != 0)) {
        unsigned int bytesInLane = SnP_laneLengthInBytes - offsetInLane;
        if (bytesInLane > sizeLeft)
            bytesInLane = sizeLeft;
        memcpy( curData, ((unsigned char *)&statesAsLanes[laneIndex(instanceIndex, lanePosition)]) + offsetInLane, bytesInLane);
        sizeLeft -= bytesInLane;
        lanePosition++;
        curData += bytesInLane;
    }

    while(sizeLeft >= SnP_laneLengthInBytes) {
        *(uint32_t*)curData = statesAsLanes[laneIndex(instanceIndex, lanePosition)];
        sizeLeft -= SnP_laneLengthInBytes;
        lanePosition++;
        curData += SnP_laneLengthInBytes;
    }

    if (sizeLeft > 0) {
        memcpy( curData, &statesAsLanes[laneIndex(instanceIndex, lanePosition)], sizeLeft);
    }
}

void Xoodootimes16_ExtractLanesAll(const void *states, unsigned char *data, unsigned int laneCount, unsigned int laneOffset)
{
    const V512 *stateAsLanes = (const V512 *)states;
    unsigned int i;
    uint32_t *data32 = (uint32_t *)data;
    V512 offsets = SET16_32(     0*laneOffset, 1*laneOffset,  2*laneOffset,  3*laneOffset,  4*laneOffset,  5*laneOffset,  6*laneOffset,  7*laneOffset, 
                                8*laneOffset, 9*laneOffset, 10*laneOffset, 11*laneOffset, 12*laneOffset, 13*laneOffset, 14*laneOffset, 15*laneOffset);

    #define Extr( argIndex )        STORE_SCATTER16_32(offsets, stateAsLanes[argIndex], &data32[argIndex])

    if ( laneCount == 12 )  {
        Extr( 0 );
        Extr( 1 );
        Extr( 2 );
        Extr( 3 );
        Extr( 4 );
        Extr( 5 );
        Extr( 6 );
        Extr( 7 );
        Extr( 8 );
        Extr( 9 );
        Extr( 10 );
        Extr( 11 );
    }
    else {
        for(i=0; i<laneCount; i++)
            Extr( i );
    }
    #undef  Extr
}

void Xoodootimes16_ExtractAndAddBytes(const void *states, unsigned int instanceIndex, const unsigned char *input, unsigned char *output, unsigned int offset, unsigned int length)
{
    unsigned int sizeLeft = length;
    unsigned int lanePosition = offset/SnP_laneLengthInBytes;
    unsigned int offsetInLane = offset%SnP_laneLengthInBytes;
    const unsigned char *curInput = input;
    unsigned char *curOutput = output;
    const uint32_t *statesAsLanes = (const uint32_t *)states;

    if ((sizeLeft > 0) && (offsetInLane != 0)) {
        unsigned int bytesInLane = SnP_laneLengthInBytes - offsetInLane;
        uint32_t lane = statesAsLanes[laneIndex(instanceIndex, lanePosition)] >> (8 * offsetInLane);
        if (bytesInLane > sizeLeft)
            bytesInLane = sizeLeft;
        sizeLeft -= bytesInLane;
        do {
            *(curOutput++) = *(curInput++) ^ (unsigned char)lane;
            lane >>= 8;
        } while ( --bytesInLane != 0);
        lanePosition++;
    }

    while(sizeLeft >= SnP_laneLengthInBytes) {
        *((uint32_t*)curOutput) = *((uint32_t*)curInput) ^ statesAsLanes[laneIndex(instanceIndex, lanePosition)];
        sizeLeft -= SnP_laneLengthInBytes;
        lanePosition++;
        curInput += SnP_laneLengthInBytes;
        curOutput += SnP_laneLengthInBytes;
    }

    if (sizeLeft != 0) {
        uint32_t lane = statesAsLanes[laneIndex(instanceIndex, lanePosition)];
        do {
            *(curOutput++) = *(curInput++) ^ (unsigned char)lane;
            lane >>= 8;
        } while ( --sizeLeft != 0);
    }
}

void Xoodootimes16_ExtractAndAddLanesAll(const void *states, const unsigned char *input, unsigned char *output, unsigned int laneCount, unsigned int laneOffset)
{
    const V512 *stateAsLanes = (const V512 *)states;
    unsigned int i;
    const uint32_t *datai32 = (const uint32_t *)input;
    uint32_t *datao32 = (uint32_t *)output;
    V512 offsets = SET16_32(     0*laneOffset, 1*laneOffset,  2*laneOffset,  3*laneOffset,  4*laneOffset,  5*laneOffset,  6*laneOffset,  7*laneOffset, 
                                8*laneOffset, 9*laneOffset, 10*laneOffset, 11*laneOffset, 12*laneOffset, 13*laneOffset, 14*laneOffset, 15*laneOffset);

    #define ExtrXor( argIndex ) STORE_SCATTER16_32(offsets, XOR( stateAsLanes[argIndex], LOAD_GATHER16_32(offsets, &datai32[argIndex])), &datao32[argIndex])

    if ( laneCount == 12 )  {
        ExtrXor( 0 );
        ExtrXor( 1 );
        ExtrXor( 2 );
        ExtrXor( 3 );
        ExtrXor( 4 );
        ExtrXor( 5 );
        ExtrXor( 6 );
        ExtrXor( 7 );
        ExtrXor( 8 );
        ExtrXor( 9 );
        ExtrXor( 10 );
        ExtrXor( 11 );
    }
    else {
        for(i=0; i<laneCount; i++)
            ExtrXor( i );
    }
    #undef  ExtrXor
}

#define DeclareVars     V512    a00, a01, a02, a03; \
                        V512    a10, a11, a12, a13; \
                        V512    a20, a21, a22, a23; \
                        V512    v1, v2;

#define State2Vars2     a00 = states[0], a01 = states[1], a02 = states[ 2], a03 = states[ 3]; \
                        a12 = states[4], a13 = states[5], a10 = states[ 6], a11 = states[ 7]; \
                        a20 = states[8], a21 = states[9], a22 = states[10], a23 = states[11]

#define State2Vars      a00 = states[0], a01 = states[1], a02 = states[ 2], a03 = states[ 3]; \
                        a10 = states[4], a11 = states[5], a12 = states[ 6], a13 = states[ 7]; \
                        a20 = states[8], a21 = states[9], a22 = states[10], a23 = states[11]

#define Vars2State      states[0] = a00, states[1] = a01, states[ 2] = a02, states[ 3] = a03; \
                        states[4] = a10, states[5] = a11, states[ 6] = a12, states[ 7] = a13; \
                        states[8] = a20, states[9] = a21, states[10] = a22, states[11] = a23

#define Round(a10i, a11i, a12i, a13i, a10w, a11w, a12w, a13w, a20i, a21i, a22i, a23i, __rc) \
                                                            \
    /* Theta: Column Parity Mixer */                        \
    /* Iota: round constants */                             \
    v1 = XOR3( a03, a13i, a23i );                           \
    v2 = XOR3( a00, a10i, a20i );                           \
    v1 = XOR( ROL32(v1, 5), ROL32(v1, 14) );               \
    a00  = XOR3( a00,  v1, CONST16_32(__rc) ); /* Iota */   \
    a10i = XOR( a10i, v1 );                                 \
    a20i = XOR( a20i, v1 );                                 \
    v1 = XOR3( a01, a11i, a21i );                           \
    v2 = XOR( ROL32(v2, 5), ROL32(v2, 14) );               \
    a01  = XOR( a01,  v2 );                                 \
    a11i = XOR( a11i, v2 );                                 \
    a21i = XOR( a21i, v2 );                                 \
    v2 = XOR3( a02, a12i, a22i );                           \
    v1 = XOR( ROL32(v1, 5), ROL32(v1, 14) );               \
    a02  = XOR( a02,  v1 );                                 \
    a12i = XOR( a12i, v1 );                                 \
    a22i = XOR( a22i, v1 );                                 \
    v2 = XOR( ROL32(v2, 5), ROL32(v2, 14) );               \
    a03  = XOR( a03,  v2 );                                 \
    a13i = XOR( a13i, v2 );                                 \
    a23i = XOR( a23i, v2 );                                 \
    Dump3("Theta",a);                                       \
                                                            \
    /* Rho-west: Plane shift */                             \
    a20i = ROL32(a20i, 11);                                 \
    a21i = ROL32(a21i, 11);                                 \
    a22i = ROL32(a22i, 11);                                 \
    a23i = ROL32(a23i, 11);                                 \
    Dump3("Rho-west",a);                                    \
                                                            \
    /* Chi: non linear step, on colums */                   \
    a00  = Chi(a00,  a10w, a20i);                           \
    a01  = Chi(a01,  a11w, a21i);                           \
    a02  = Chi(a02,  a12w, a22i);                           \
    a03  = Chi(a03,  a13w, a23i);                           \
    a10w = Chi(a10w, a20i, a00);                            \
    a11w = Chi(a11w, a21i, a01);                            \
    a12w = Chi(a12w, a22i, a02);                            \
    a13w = Chi(a13w, a23i, a03);                            \
    a20i = Chi(a20i, a00,  a10w);                           \
    a21i = Chi(a21i, a01,  a11w);                           \
    a22i = Chi(a22i, a02,  a12w);                           \
    a23i = Chi(a23i, a03,  a13w);                           \
    Dump3("Chi",a);                                         \
                                                            \
    /* Rho-east: Plane shift */                             \
    a10w = ROL32(a10w, 1);                                  \
    a11w = ROL32(a11w, 1);                                  \
    a12w = ROL32(a12w, 1);                                  \
    a13w = ROL32(a13w, 1);                                  \
    a20i = ROL32(a20i, 8);                                  \
    a21i = ROL32(a21i, 8);                                  \
    a22i = ROL32(a22i, 8);                                  \
    a23i = ROL32(a23i, 8);                                  \
    Dump3("Rho-east",a)

void Xoodootimes16_PermuteAll_6rounds(void *argstates)
{
    V512 * states = (V512 *)argstates;
    DeclareVars;

    State2Vars2;
    Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc6 );
    Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc5 );
    Round(  a10, a11, a12, a13,    a13, a10, a11, a12,    a20, a21, a22, a23,    _rc4 );
    Round(  a13, a10, a11, a12,    a12, a13, a10, a11,    a22, a23, a20, a21,    _rc3 );
    Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc2 );
    Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc1 );
    Dump2("Permutation\n", a);
    Vars2State;
}

void Xoodootimes16_PermuteAll_12rounds(void *argstates)
{
    V512 * states = (V512 *)argstates;
    DeclareVars;

    State2Vars;
    Round(  a10, a11, a12, a13,    a13, a10, a11, a12,    a20, a21, a22, a23,    _rc12 );
    Round(  a13, a10, a11, a12,    a12, a13, a10, a11,    a22, a23, a20, a21,    _rc11 );
    Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc10 );
    Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc9 );
    Round(  a10, a11, a12, a13,    a13, a10, a11, a12,    a20, a21, a22, a23,    _rc8 );
    Round(  a13, a10, a11, a12,    a12, a13, a10, a11,    a22, a23, a20, a21,    _rc7 );
    Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc6 );
    Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc5 );
    Round(  a10, a11, a12, a13,    a13, a10, a11, a12,    a20, a21, a22, a23,    _rc4 );
    Round(  a13, a10, a11, a12,    a12, a13, a10, a11,    a22, a23, a20, a21,    _rc3 );
    Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc2 );
    Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc1 );
    Dump2("Permutation\n", a);
    Vars2State;
}

void Xooffftimes16_AddIs(unsigned char *output, const unsigned char *input, size_t bitLen)
{
    size_t  byteLen = bitLen / 8;
    V512    lanes1, lanes2, lanes3, lanes4;
    V256    lanesA, lanesB;

    while ( byteLen >= 128 ) {
        lanes1 = LOAD512u(input[ 0]);
        lanes2 = LOAD512u(input[64]);
        lanes3 = LOAD512u(output[ 0]);
        lanes4 = LOAD512u(output[64]);
        lanes1 = XOR(lanes1, lanes3);
        lanes2 = XOR(lanes2, lanes4);
        STORE512u(output[ 0], lanes1);
        STORE512u(output[64], lanes2);
        input += 128;
        output += 128;
        byteLen -= 128;
    }
    while ( byteLen >= 32 ) {
        lanesA = LOAD256u(input[0]);
        lanesB = LOAD256u(output[0]);
        input += 32;
        lanesA = XOR256(lanesA, lanesB);
        byteLen -= 32;
        STORE256u(output[0], lanesA);
        output += 32;
    }
   while ( byteLen >= 8 ) {
        *((uint64_t*)output) ^= *((uint64_t*)input);
        input += 8;
        output += 8;
        byteLen -= 8;
    }
    while ( byteLen-- != 0 ) {
        *output++ ^= *input++;
    }

    bitLen &= 7;
    if (bitLen != 0)
    {
        *output ^= *input;
        *output &= (1 << bitLen) - 1;
    }
}

size_t Xooffftimes16_CompressFastLoop(unsigned char *k, unsigned char *x, const unsigned char *input, size_t length)
{
    DeclareVars;
    uint32_t    *k32 = (uint32_t*)k;
    uint32_t    *x32 = (uint32_t*)x;
    uint32_t    *i32 = (uint32_t*)input;
    size_t      initialLength;
    V512        rCGKDHLEIcgkdhlei;
    V512        offsets;
    V512        x00, x01, x02, x03, x10, x11, x12, x13, x20, x21, x22, x23;

    DUMP32("k32",k32);
    rCGKDHLEIcgkdhlei = LOAD_GATHER16_32(*(const V512*)oGatherScatterOffsetsRoll, k32);
    offsets = *(V512*)oGatherScatterOffsets;

    x00 = _mm512_setzero_si512();
    x01 = _mm512_setzero_si512();
    x02 = _mm512_setzero_si512();
    x03 = _mm512_setzero_si512();
    x10 = _mm512_setzero_si512();
    x11 = _mm512_setzero_si512();
    x12 = _mm512_setzero_si512();
    x13 = _mm512_setzero_si512();
    x20 = _mm512_setzero_si512();
    x21 = _mm512_setzero_si512();
    x22 = _mm512_setzero_si512();
    x23 = _mm512_setzero_si512();
    initialLength = length;
    do {
        /* a10 - a12 and a11 - a13 are swapped */
        a00 = SHUFFLE_LANES_RIGHT(*(const V512*)oshuffleR_4, rCGKDHLEIcgkdhlei);
        a12 = SHUFFLE_LANES_RIGHT(*(const V512*)oshuffleR_1, a00);
        rCGKDHLEIcgkdhlei = XOR3(a00, SHL32(a00, 13), ROL32(a12, 3)); /* only lanes 0 - A are ok */

        a00 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllLow12_0, rCGKDHLEIcgkdhlei);
        a12 = _mm512_permutex2var_epi32(a12, *(const V512*)oAllLow11_0, rCGKDHLEIcgkdhlei);
        rCGKDHLEIcgkdhlei = XOR3(a00, SHL32(a00, 13), ROL32(a12, 3)); /* All lanes are ok */
        DumpLane("rCGKD", rCGKDHLEIcgkdhlei);

        a01 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllFrom3_4, rCGKDHLEIcgkdhlei);
        a02 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllFrom6_4, rCGKDHLEIcgkdhlei);
        a03 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllFrom9_4, rCGKDHLEIcgkdhlei);
            
        a13 = _mm512_permutex2var_epi32(a01, *(const V512*)oAllFrom1_7,  rCGKDHLEIcgkdhlei);
        a10 = _mm512_permutex2var_epi32(a02, *(const V512*)oAllFrom1_10, rCGKDHLEIcgkdhlei);
        a11 = _mm512_permutex2var_epi32(a03, *(const V512*)oAllFrom1_13, rCGKDHLEIcgkdhlei);
        
        a20 = _mm512_permutex2var_epi32(a12, *(const V512*)oAllFrom1_5,  rCGKDHLEIcgkdhlei);
        a21 = _mm512_permutex2var_epi32(a13, *(const V512*)oAllFrom1_8,  rCGKDHLEIcgkdhlei);
        a22 = _mm512_permutex2var_epi32(a10, *(const V512*)oAllFrom1_11, rCGKDHLEIcgkdhlei);
        a23 = _mm512_permutex2var_epi32(a11, *(const V512*)oAllFrom1_14, rCGKDHLEIcgkdhlei);
        Dump("Roll-c", a);

        a00 = XOR( a00, LOAD_GATHER16_32(offsets, i32+0));
        a01 = XOR( a01, LOAD_GATHER16_32(offsets, i32+1));
        a02 = XOR( a02, LOAD_GATHER16_32(offsets, i32+2));
        a03 = XOR( a03, LOAD_GATHER16_32(offsets, i32+3));
        a12 = XOR( a12, LOAD_GATHER16_32(offsets, i32+4));
        a13 = XOR( a13, LOAD_GATHER16_32(offsets, i32+5));
        a10 = XOR( a10, LOAD_GATHER16_32(offsets, i32+6));
        a11 = XOR( a11, LOAD_GATHER16_32(offsets, i32+7));
        a20 = XOR( a20, LOAD_GATHER16_32(offsets, i32+8));
        a21 = XOR( a21, LOAD_GATHER16_32(offsets, i32+9));
        a22 = XOR( a22, LOAD_GATHER16_32(offsets, i32+10));
        a23 = XOR( a23, LOAD_GATHER16_32(offsets, i32+11));
        Dump("Input Xoodoo (after add)", a);

        Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc6 );
        Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc5 );
        Round(  a10, a11, a12, a13,    a13, a10, a11, a12,    a20, a21, a22, a23,    _rc4 );
        Round(  a13, a10, a11, a12,    a12, a13, a10, a11,    a22, a23, a20, a21,    _rc3 );
        Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc2 );
        Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc1 );
        Dump("Output Xoodoo", a);

        x00 = XOR(x00, a00);
        x01 = XOR(x01, a01);
        x02 = XOR(x02, a02);
        x03 = XOR(x03, a03);
        x10 = XOR(x10, a10);
        x11 = XOR(x11, a11);
        x12 = XOR(x12, a12);
        x13 = XOR(x13, a13);
        x20 = XOR(x20, a20);
        x21 = XOR(x21, a21);
        x22 = XOR(x22, a22);
        x23 = XOR(x23, a23);
        Dump("Accu x", x);

        i32 += NLANES*16;
        length -= NLANES*4*16;
    }
    while (length >= (NLANES*4*16));

    /*    Reduce from lanes 16 to 8 */
    v1 = *(V512*)oLow256;
    v2 = *(V512*)oHigh256;
    x00 = XOR(_mm512_permutex2var_epi32(x00, v1, x10), _mm512_permutex2var_epi32(x00, v2, x10));
    x01 = XOR(_mm512_permutex2var_epi32(x01, v1, x11), _mm512_permutex2var_epi32(x01, v2, x11));
    x02 = XOR(_mm512_permutex2var_epi32(x02, v1, x12), _mm512_permutex2var_epi32(x02, v2, x12));
    x03 = XOR(_mm512_permutex2var_epi32(x03, v1, x13), _mm512_permutex2var_epi32(x03, v2, x13));
    x20 = XOR(_mm512_permutex2var_epi32(x20, v1, x22), _mm512_permutex2var_epi32(x20, v2, x22));
    x21 = XOR(_mm512_permutex2var_epi32(x21, v1, x23), _mm512_permutex2var_epi32(x21, v2, x23));

    /*    Reduce from 8 lanes to 4 */
    v1 = *( V512*)oLow128;
    v2 = *( V512*)oHigh128;
    x00 = XOR(_mm512_permutex2var_epi32(x00, v1, x02), _mm512_permutex2var_epi32(x00, v2, x02));
    x01 = XOR(_mm512_permutex2var_epi32(x01, v1, x03), _mm512_permutex2var_epi32(x01, v2, x03));
    x20 = XOR(_mm512_permutex2var_epi32(x20, v1, x21), _mm512_permutex2var_epi32(x20, v2, x21));

    /*    Reduce from 4 lanes to 2 */
    v1 = *(V512*)oLow64;
    v2 = *(V512*)oHigh64;
    x00 = XOR(_mm512_permutex2var_epi32(x00, v1, x01), _mm512_permutex2var_epi32(x00, v2, x01));
    x20 = XOR(_mm512_permutex2var_epi32(x20, v1, x20), _mm512_permutex2var_epi32(x20, v2, x20));

    /*    Reduce from 2 lanes to 1 */
    x00 = XOR(_mm512_permutex2var_epi32(x00, *(V512*)oLow32, x20), _mm512_permutex2var_epi32(x00, *(V512*)oHigh32, x20));

    /*  load xAccu, xor and store 12 lanes */
    x01 = _mm512_maskz_load_epi64(0x3F, x32);
    x00 = XOR(x00, x01);
    _mm512_mask_store_epi64(x32, 0x3F, x00);
    DUMP32( "x32", x32);

    /*    Save new k */
    _mm512_mask_i32scatter_epi32(k32, 0xFFF0, *(const V512*)oGatherScatterOffsetsRoll, rCGKDHLEIcgkdhlei, 4);
    DUMP32( "k32", k32);

    return initialLength - length;
}

size_t Xooffftimes16_ExpandFastLoop(unsigned char *yAccu, const unsigned char *kRoll, unsigned char *output, size_t length)
{
    DeclareVars;
    uint32_t    *k32 = (uint32_t*)kRoll;
    uint32_t    *y32 = (uint32_t*)yAccu;
    uint32_t    *o32 = (uint32_t*)output;
    size_t      initialLength;
    V512        rCGKDHLEIcgkdhlei;
    V512        offsets;

    rCGKDHLEIcgkdhlei = LOAD_GATHER16_32(*(const V512*)oGatherScatterOffsetsRoll, y32);
    offsets = *(const V512*)oGatherScatterOffsets;

    initialLength = length;
    do {
        /* a10 - a12 and a11 - a13 are swapped */
        a00 = SHUFFLE_LANES_RIGHT(*(const V512*)oshuffleR_4, rCGKDHLEIcgkdhlei);
        a12 = SHUFFLE_LANES_RIGHT(*(const V512*)oshuffleR_1, a00); /*  */
        a20 = SHUFFLE_LANES_RIGHT(*(const V512*)oshuffleR_2, a00); /*  */
        a23 = AND(a20, a12); /* a23 used as temporary var */
        rCGKDHLEIcgkdhlei = XOR3(ROL32(a00, 5), ROL32(a12, 13), a23); /* Only lanes 0 - 9 are ok */
        rCGKDHLEIcgkdhlei = XOR(rCGKDHLEIcgkdhlei, CONST16_32(7));

        a00 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllLow12_0, rCGKDHLEIcgkdhlei);
        a12 = _mm512_permutex2var_epi32(a12, *(const V512*)oAllLow11_0, rCGKDHLEIcgkdhlei);
        a20 = _mm512_permutex2var_epi32(a20, *(const V512*)oAllLow10_0, rCGKDHLEIcgkdhlei);
        a23 = AND(a20, a12); /* a23 used as temporary var */
        rCGKDHLEIcgkdhlei = XOR3(ROL32(a00, 5), ROL32(a12, 13), a23); /* All lanes are ok */
        rCGKDHLEIcgkdhlei = XOR(rCGKDHLEIcgkdhlei, CONST16_32(7));

        a01 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllFrom3_4, rCGKDHLEIcgkdhlei);
        a02 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllFrom6_4, rCGKDHLEIcgkdhlei);
        a03 = _mm512_permutex2var_epi32(a00, *(const V512*)oAllFrom9_4, rCGKDHLEIcgkdhlei);
            
        a13 = _mm512_permutex2var_epi32(a01, *(const V512*)oAllFrom1_7,  rCGKDHLEIcgkdhlei); /*  */
        a10 = _mm512_permutex2var_epi32(a02, *(const V512*)oAllFrom1_10, rCGKDHLEIcgkdhlei); /*  */
        a11 = _mm512_permutex2var_epi32(a03, *(const V512*)oAllFrom1_13, rCGKDHLEIcgkdhlei); /*  */
        
        a21 = _mm512_permutex2var_epi32(a13, *(const V512*)oAllFrom1_8,  rCGKDHLEIcgkdhlei); /*  */
        a22 = _mm512_permutex2var_epi32(a10, *(const V512*)oAllFrom1_11, rCGKDHLEIcgkdhlei); /*  */
        a23 = _mm512_permutex2var_epi32(a11, *(const V512*)oAllFrom1_14, rCGKDHLEIcgkdhlei); /*  */
        Dump("Roll-e", a);

        Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc6 );
        Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc5 );
        Round(  a10, a11, a12, a13,    a13, a10, a11, a12,    a20, a21, a22, a23,    _rc4 );
        Round(  a13, a10, a11, a12,    a12, a13, a10, a11,    a22, a23, a20, a21,    _rc3 );
        Round(  a12, a13, a10, a11,    a11, a12, a13, a10,    a20, a21, a22, a23,    _rc2 );
        Round(  a11, a12, a13, a10,    a10, a11, a12, a13,    a22, a23, a20, a21,    _rc1 );
        Dump("Xoodoo(y)", a);

        a00 = XOR(a00, CONST16_32(k32[0]));
        a01 = XOR(a01, CONST16_32(k32[1]));
        a02 = XOR(a02, CONST16_32(k32[2]));
        a03 = XOR(a03, CONST16_32(k32[3]));
        a10 = XOR(a10, CONST16_32(k32[4]));
        a11 = XOR(a11, CONST16_32(k32[5]));
        a12 = XOR(a12, CONST16_32(k32[6]));
        a13 = XOR(a13, CONST16_32(k32[7]));
        a20 = XOR(a20, CONST16_32(k32[8]));
        a21 = XOR(a21, CONST16_32(k32[9]));
        a22 = XOR(a22, CONST16_32(k32[10]));
        a23 = XOR(a23, CONST16_32(k32[11]));
        Dump("Xoodoo(y) + kRoll", a);

        /*  Extract */
        STORE_SCATTER16_32(offsets, a00, o32+0);
        STORE_SCATTER16_32(offsets, a01, o32+1);
        STORE_SCATTER16_32(offsets, a02, o32+2);
        STORE_SCATTER16_32(offsets, a03, o32+3);
        STORE_SCATTER16_32(offsets, a10, o32+4);
        STORE_SCATTER16_32(offsets, a11, o32+5);
        STORE_SCATTER16_32(offsets, a12, o32+6);
        STORE_SCATTER16_32(offsets, a13, o32+7);
        STORE_SCATTER16_32(offsets, a20, o32+8);
        STORE_SCATTER16_32(offsets, a21, o32+9);
        STORE_SCATTER16_32(offsets, a22, o32+10);
        STORE_SCATTER16_32(offsets, a23, o32+11);

        o32 += NLANES*16;
        length -= NLANES*4*16;
    }
    while (length >= (NLANES*4*16));

    /* Save new y */
    _mm512_mask_i32scatter_epi32(y32, 0xFFF0, *(const V512*)oGatherScatterOffsetsRoll, rCGKDHLEIcgkdhlei, 4);
    DUMP32( "y32", y32);

    return initialLength - length;
}
