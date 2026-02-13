//-------------------------------------------------------------------------------------
// BC6HBC7.cpp
//  
// Block-compression (BC) functionality for BC6H and BC7 (DirectX 11 texture compression)
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//  
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
//-------------------------------------------------------------------------------------

#include "directxtexp.h"

#include "BC.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

//-------------------------------------------------------------------------------------
// Macros
//-------------------------------------------------------------------------------------

#define SIGN_EXTEND(x,nb) ((((x)&(1<<((nb)-1)))?((~0)<<(nb)):0)|(x))

// Because these are used in SAL annotations, they need to remain macros rather than const values
#define BC6H_MAX_REGIONS 2
#define BC6H_MAX_INDICES 16
#define BC7_MAX_REGIONS 3
#define BC7_MAX_INDICES 16

namespace
{
    //-------------------------------------------------------------------------------------
    // Constants
    //-------------------------------------------------------------------------------------

    const uint16_t F16S_MASK = 0x8000;   // f16 sign mask
    const uint16_t F16EM_MASK = 0x7fff;   // f16 exp & mantissa mask
    const uint16_t F16MAX = 0x7bff;   // MAXFLT bit pattern for XMHALF

    const size_t BC6H_NUM_CHANNELS = 3;
    const size_t BC6H_MAX_SHAPES = 32;

    const size_t BC7_NUM_CHANNELS = 4;
    const size_t BC7_MAX_SHAPES = 64;

    const int32_t BC67_WEIGHT_MAX = 64;
    const uint32_t BC67_WEIGHT_SHIFT = 6;
    const int32_t BC67_WEIGHT_ROUND = 32;

    const float fEpsilon = (0.25f / 64.0f) * (0.25f / 64.0f);
    const float pC3[] = { 2.0f / 2.0f, 1.0f / 2.0f, 0.0f / 2.0f };
    const float pD3[] = { 0.0f / 2.0f, 1.0f / 2.0f, 2.0f / 2.0f };
    const float pC4[] = { 3.0f / 3.0f, 2.0f / 3.0f, 1.0f / 3.0f, 0.0f / 3.0f };
    const float pD4[] = { 0.0f / 3.0f, 1.0f / 3.0f, 2.0f / 3.0f, 3.0f / 3.0f };

    // Partition, Shape, Pixel (index into 4x4 block)
    const uint8_t g_aPartitionTable[3][64][16] =
    {
        {   // 1 Region case has no subsets (all 0)
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
        },

        {   // BC6H/BC7 Partition Set for 2 Subsets
            { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1 }, // Shape 0
            { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 }, // Shape 1
            { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 }, // Shape 2
            { 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 3
            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1 }, // Shape 4
            { 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 5
            { 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 6
            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 7
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1 }, // Shape 8
            { 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 9
            { 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1 }, // Shape 10
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1 }, // Shape 11
            { 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 12
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 13
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 14
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 }, // Shape 15
            { 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1 }, // Shape 16
            { 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // Shape 17
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0 }, // Shape 18
            { 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0 }, // Shape 19
            { 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, // Shape 20
            { 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0 }, // Shape 21
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0 }, // Shape 22
            { 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1 }, // Shape 23
            { 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0 }, // Shape 24
            { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0 }, // Shape 25
            { 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0 }, // Shape 26
            { 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0 }, // Shape 27
            { 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0 }, // Shape 28
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // Shape 29
            { 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0 }, // Shape 30
            { 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0 }, // Shape 31

                                                                // BC7 Partition Set for 2 Subsets (second-half)
            { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 }, // Shape 32
            { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1 }, // Shape 33
            { 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0 }, // Shape 34
            { 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0 }, // Shape 35
            { 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0 }, // Shape 36
            { 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0 }, // Shape 37
            { 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1 }, // Shape 38
            { 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1 }, // Shape 39
            { 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0 }, // Shape 40
            { 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0 }, // Shape 41
            { 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0 }, // Shape 42
            { 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0 }, // Shape 43
            { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 }, // Shape 44
            { 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1 }, // Shape 45
            { 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1 }, // Shape 46
            { 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0 }, // Shape 47
            { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 }, // Shape 48
            { 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0 }, // Shape 49
            { 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0 }, // Shape 50
            { 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0 }, // Shape 51
            { 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1 }, // Shape 52
            { 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1 }, // Shape 53
            { 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0 }, // Shape 54
            { 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0 }, // Shape 55
            { 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1 }, // Shape 56
            { 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1 }, // Shape 57
            { 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1 }, // Shape 58
            { 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1 }, // Shape 59
            { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1 }, // Shape 60
            { 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 }, // Shape 61
            { 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0 }, // Shape 62
            { 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1 }  // Shape 63
        },

        {   // BC7 Partition Set for 3 Subsets
            { 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 1, 2, 2, 2, 2 }, // Shape 0
            { 0, 0, 0, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 2, 1 }, // Shape 1
            { 0, 0, 0, 0, 2, 0, 0, 1, 2, 2, 1, 1, 2, 2, 1, 1 }, // Shape 2
            { 0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 1, 0, 1, 1, 1 }, // Shape 3
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2 }, // Shape 4
            { 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 2, 2 }, // Shape 5
            { 0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 }, // Shape 6
            { 0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1 }, // Shape 7
            { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2 }, // Shape 8
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2 }, // Shape 9
            { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 10
            { 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2 }, // Shape 11
            { 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2 }, // Shape 12
            { 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2 }, // Shape 13
            { 0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 2 }, // Shape 14
            { 0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0, 2, 2, 2, 0 }, // Shape 15
            { 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2 }, // Shape 16
            { 0, 1, 1, 1, 0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0 }, // Shape 17
            { 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2 }, // Shape 18
            { 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1 }, // Shape 19
            { 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2 }, // Shape 20
            { 0, 0, 0, 1, 0, 0, 0, 1, 2, 2, 2, 1, 2, 2, 2, 1 }, // Shape 21
            { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2 }, // Shape 22
            { 0, 0, 0, 0, 1, 1, 0, 0, 2, 2, 1, 0, 2, 2, 1, 0 }, // Shape 23
            { 0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1, 0, 0, 0, 0 }, // Shape 24
            { 0, 0, 1, 2, 0, 0, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2 }, // Shape 25
            { 0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1, 0, 1, 1, 0 }, // Shape 26
            { 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1 }, // Shape 27
            { 0, 0, 2, 2, 1, 1, 0, 2, 1, 1, 0, 2, 0, 0, 2, 2 }, // Shape 28
            { 0, 1, 1, 0, 0, 1, 1, 0, 2, 0, 0, 2, 2, 2, 2, 2 }, // Shape 29
            { 0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1 }, // Shape 30
            { 0, 0, 0, 0, 2, 0, 0, 0, 2, 2, 1, 1, 2, 2, 2, 1 }, // Shape 31
            { 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 2, 2, 2 }, // Shape 32
            { 0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 2, 0, 0, 1, 1 }, // Shape 33
            { 0, 0, 1, 1, 0, 0, 1, 2, 0, 0, 2, 2, 0, 2, 2, 2 }, // Shape 34
            { 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0 }, // Shape 35
            { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0 }, // Shape 36
            { 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0 }, // Shape 37
            { 0, 1, 2, 0, 2, 0, 1, 2, 1, 2, 0, 1, 0, 1, 2, 0 }, // Shape 38
            { 0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 1, 1 }, // Shape 39
            { 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 1, 1 }, // Shape 40
            { 0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 41
            { 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1 }, // Shape 42
            { 0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2, 1, 1, 2, 2 }, // Shape 43
            { 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 1, 1 }, // Shape 44
            { 0, 2, 2, 0, 1, 2, 2, 1, 0, 2, 2, 0, 1, 2, 2, 1 }, // Shape 45
            { 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 1 }, // Shape 46
            { 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1 }, // Shape 47
            { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2 }, // Shape 48
            { 0, 2, 2, 2, 0, 1, 1, 1, 0, 2, 2, 2, 0, 1, 1, 1 }, // Shape 49
            { 0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2 }, // Shape 50
            { 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2 }, // Shape 51
            { 0, 2, 2, 2, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2 }, // Shape 52
            { 0, 0, 0, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2 }, // Shape 53
            { 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2 }, // Shape 54
            { 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2 }, // Shape 55
            { 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 56
            { 0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2 }, // Shape 57
            { 0, 0, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2 }, // Shape 58
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2 }, // Shape 59
            { 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1 }, // Shape 60
            { 0, 2, 2, 2, 1, 2, 2, 2, 0, 2, 2, 2, 1, 2, 2, 2 }, // Shape 61
            { 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 }, // Shape 62
            { 0, 1, 1, 1, 2, 0, 1, 1, 2, 2, 0, 1, 2, 2, 2, 0 }  // Shape 63
        }
    };

    // Partition, Shape, Fixup
    const uint8_t g_aFixUp[3][64][3] =
    {
        {   // No fix-ups for 1st subset for BC6H or BC7
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },
            { 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 },{ 0, 0, 0 }
        },

        {   // BC6H/BC7 Partition Set Fixups for 2 Subsets
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0, 8, 0 },{ 0,15, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
            { 0, 8, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },

            // BC7 Partition Set Fixups for 2 Subsets (second-half)
            { 0,15, 0 },{ 0,15, 0 },{ 0, 6, 0 },{ 0, 8, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0, 2, 0 },{ 0, 8, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
            { 0, 2, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0, 6, 0 },
            { 0, 6, 0 },{ 0, 2, 0 },{ 0, 6, 0 },{ 0, 8, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0, 2, 0 },{ 0, 2, 0 },
            { 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },{ 0,15, 0 },
            { 0,15, 0 },{ 0, 2, 0 },{ 0, 2, 0 },{ 0,15, 0 }
        },

        {   // BC7 Partition Set Fixups for 3 Subsets
            { 0, 3,15 },{ 0, 3, 8 },{ 0,15, 8 },{ 0,15, 3 },
            { 0, 8,15 },{ 0, 3,15 },{ 0,15, 3 },{ 0,15, 8 },
            { 0, 8,15 },{ 0, 8,15 },{ 0, 6,15 },{ 0, 6,15 },
            { 0, 6,15 },{ 0, 5,15 },{ 0, 3,15 },{ 0, 3, 8 },
            { 0, 3,15 },{ 0, 3, 8 },{ 0, 8,15 },{ 0,15, 3 },
            { 0, 3,15 },{ 0, 3, 8 },{ 0, 6,15 },{ 0,10, 8 },
            { 0, 5, 3 },{ 0, 8,15 },{ 0, 8, 6 },{ 0, 6,10 },
            { 0, 8,15 },{ 0, 5,15 },{ 0,15,10 },{ 0,15, 8 },
            { 0, 8,15 },{ 0,15, 3 },{ 0, 3,15 },{ 0, 5,10 },
            { 0, 6,10 },{ 0,10, 8 },{ 0, 8, 9 },{ 0,15,10 },
            { 0,15, 6 },{ 0, 3,15 },{ 0,15, 8 },{ 0, 5,15 },
            { 0,15, 3 },{ 0,15, 6 },{ 0,15, 6 },{ 0,15, 8 },
            { 0, 3,15 },{ 0,15, 3 },{ 0, 5,15 },{ 0, 5,15 },
            { 0, 5,15 },{ 0, 8,15 },{ 0, 5,15 },{ 0,10,15 },
            { 0, 5,15 },{ 0,10,15 },{ 0, 8,15 },{ 0,13,15 },
            { 0,15, 3 },{ 0,12,15 },{ 0, 3,15 },{ 0, 3, 8 }
        }
    };

    const int g_aWeights2[] = { 0, 21, 43, 64 };
    const int g_aWeights3[] = { 0, 9, 18, 27, 37, 46, 55, 64 };
    const int g_aWeights4[] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };
}

namespace DirectX
{
    class LDRColorA
    {
    public:
        uint8_t r, g, b, a;

        LDRColorA() = default;
        LDRColorA(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}

        const uint8_t& operator [] (_In_range_(0, 3) size_t uElement) const
        {
            switch (uElement)
            {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            case 3: return a;
            default: assert(false); return r;
            }
        }

        uint8_t& operator [] (_In_range_(0, 3) size_t uElement)
        {
            switch (uElement)
            {
            case 0: return r;
            case 1: return g;
            case 2: return b;
            case 3: return a;
            default: assert(false); return r;
            }
        }

        LDRColorA operator = (_In_ const HDRColorA& c)
        {
            LDRColorA ret;
            HDRColorA tmp(c);
            tmp = tmp.Clamp(0.0f, 1.0f) * 255.0f;
            ret.r = uint8_t(tmp.r + 0.001f);
            ret.g = uint8_t(tmp.g + 0.001f);
            ret.b = uint8_t(tmp.b + 0.001f);
            ret.a = uint8_t(tmp.a + 0.001f);
            return ret;
        }

        static void InterpolateRGB(_In_ const LDRColorA& c0, _In_ const LDRColorA& c1, _In_ size_t wc, _In_ _In_range_(2, 4) size_t wcprec, _Out_ LDRColorA& out)
        {
            const int* aWeights = nullptr;
            switch (wcprec)
            {
            case 2: aWeights = g_aWeights2; assert(wc < 4); _Analysis_assume_(wc < 4); break;
            case 3: aWeights = g_aWeights3; assert(wc < 8); _Analysis_assume_(wc < 8); break;
            case 4: aWeights = g_aWeights4; assert(wc < 16); _Analysis_assume_(wc < 16); break;
            default: assert(false); out.r = out.g = out.b = 0; return;
            }
            out.r = uint8_t((uint32_t(c0.r) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.r) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
            out.g = uint8_t((uint32_t(c0.g) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.g) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
            out.b = uint8_t((uint32_t(c0.b) * uint32_t(BC67_WEIGHT_MAX - aWeights[wc]) + uint32_t(c1.b) * uint32_t(aWeights[wc]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
        }

        static void InterpolateA(_In_ const LDRColorA& c0, _In_ const LDRColorA& c1, _In_ size_t wa, _In_range_(2, 4) _In_ size_t waprec, _Out_ LDRColorA& out)
        {
            const int* aWeights = nullptr;
            switch (waprec)
            {
            case 2: aWeights = g_aWeights2; assert(wa < 4); _Analysis_assume_(wa < 4); break;
            case 3: aWeights = g_aWeights3; assert(wa < 8); _Analysis_assume_(wa < 8); break;
            case 4: aWeights = g_aWeights4; assert(wa < 16); _Analysis_assume_(wa < 16); break;
            default: assert(false); out.a = 0; return;
            }
            out.a = uint8_t((uint32_t(c0.a) * uint32_t(BC67_WEIGHT_MAX - aWeights[wa]) + uint32_t(c1.a) * uint32_t(aWeights[wa]) + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT);
        }

        static void Interpolate(_In_ const LDRColorA& c0, _In_ const LDRColorA& c1, _In_ size_t wc, _In_ size_t wa, _In_ _In_range_(2, 4) size_t wcprec, _In_ _In_range_(2, 4) size_t waprec, _Out_ LDRColorA& out)
        {
            InterpolateRGB(c0, c1, wc, wcprec, out);
            InterpolateA(c0, c1, wa, waprec, out);
        }
    };

    static_assert(sizeof(LDRColorA) == 4, "Unexpected packing");

    struct LDREndPntPair
    {
        LDRColorA A;
        LDRColorA B;
    };

    inline HDRColorA::HDRColorA(const LDRColorA& c)
    {
        r = float(c.r) * (1.0f / 255.0f);
        g = float(c.g) * (1.0f / 255.0f);
        b = float(c.b) * (1.0f / 255.0f);
        a = float(c.a) * (1.0f / 255.0f);
    }

    inline HDRColorA& HDRColorA::operator = (const LDRColorA& c)
    {
        r = (float)c.r;
        g = (float)c.g;
        b = (float)c.b;
        a = (float)c.a;
        return *this;
    }

    inline LDRColorA HDRColorA::ToLDRColorA() const
    {
        return LDRColorA((uint8_t)(r + 0.01f), (uint8_t)(g + 0.01f), (uint8_t)(b + 0.01f), (uint8_t)(a + 0.01f));
    }
}

namespace
{
    class INTColor
    {
    public:
        int r, g, b;
        int pad;

    public:
        INTColor() = default;
        INTColor(int nr, int ng, int nb) { r = nr; g = ng; b = nb; }
        INTColor(const INTColor& c) { r = c.r; g = c.g; b = c.b; }

        INTColor operator - (_In_ const INTColor& c) const
        {
            return INTColor(r - c.r, g - c.g, b - c.b);
        }

        INTColor& operator += (_In_ const INTColor& c)
        {
            r += c.r;
            g += c.g;
            b += c.b;
            return *this;
        }

        INTColor& operator -= (_In_ const INTColor& c)
        {
            r -= c.r;
            g -= c.g;
            b -= c.b;
            return *this;
        }

        INTColor& operator &= (_In_ const INTColor& c)
        {
            r &= c.r;
            g &= c.g;
            b &= c.b;
            return *this;
        }

        int& operator [] (_In_ uint8_t i)
        {
            assert(i < sizeof(INTColor) / sizeof(int));
            _Analysis_assume_(i < sizeof(INTColor) / sizeof(int));
            return ((int*) this)[i];
        }

        void Set(_In_ const HDRColorA& c, _In_ bool bSigned)
        {
            PackedVector::XMHALF4 aF16;

            XMVECTOR v = XMLoadFloat4((const XMFLOAT4*)& c);
            XMStoreHalf4(&aF16, v);

            r = F16ToINT(aF16.x, bSigned);
            g = F16ToINT(aF16.y, bSigned);
            b = F16ToINT(aF16.z, bSigned);
        }

        INTColor& Clamp(_In_ int iMin, _In_ int iMax)
        {
            r = std::min<int>(iMax, std::max<int>(iMin, r));
            g = std::min<int>(iMax, std::max<int>(iMin, g));
            b = std::min<int>(iMax, std::max<int>(iMin, b));
            return *this;
        }

        INTColor& SignExtend(_In_ const LDRColorA& Prec)
        {
            r = SIGN_EXTEND(r, Prec.r);
            g = SIGN_EXTEND(g, Prec.g);
            b = SIGN_EXTEND(b, Prec.b);
            return *this;
        }

        void ToF16(_Out_writes_(3) PackedVector::HALF aF16[3], _In_ bool bSigned) const
        {
            aF16[0] = INT2F16(r, bSigned);
            aF16[1] = INT2F16(g, bSigned);
            aF16[2] = INT2F16(b, bSigned);
        }

    private:
        static int F16ToINT(_In_ const PackedVector::HALF& f, _In_ bool bSigned)
        {
            uint16_t input = *((const uint16_t*)&f);
            int out, s;
            if (bSigned)
            {
                s = input & F16S_MASK;
                input &= F16EM_MASK;
                if (input > F16MAX) out = F16MAX;
                else out = input;
                out = s ? -out : out;
            }
            else
            {
                if (input & F16S_MASK) out = 0;
                else out = input;
            }
            return out;
        }

        static PackedVector::HALF INT2F16(_In_ int input, _In_ bool bSigned)
        {
            PackedVector::HALF h;
            uint16_t out;
            if (bSigned)
            {
                int s = 0;
                if (input < 0)
                {
                    s = F16S_MASK;
                    input = -input;
                }
                out = uint16_t(s | input);
            }
            else
            {
                assert(input >= 0 && input <= F16MAX);
                out = (uint16_t)input;
            }

            *((uint16_t*)&h) = out;
            return h;
        }
    };

    static_assert(sizeof(INTColor) == 16, "Unexpected packing");

    struct INTEndPntPair
    {
        INTColor A;
        INTColor B;
    };

    template< size_t SizeInBytes >
    class CBits
    {
    public:
        uint8_t GetBit(_Inout_ size_t& uStartBit) const
        {
            assert(uStartBit < 128);
            _Analysis_assume_(uStartBit < 128);
            size_t uIndex = uStartBit >> 3;
            uint8_t ret = (m_uBits[uIndex] >> (uStartBit - (uIndex << 3))) & 0x01;
            uStartBit++;
            return ret;
        }

        uint8_t GetBits(_Inout_ size_t& uStartBit, _In_ size_t uNumBits) const
        {
            if (uNumBits == 0) return 0;
            assert(uStartBit + uNumBits <= 128 && uNumBits <= 8);
            _Analysis_assume_(uStartBit + uNumBits <= 128 && uNumBits <= 8);
            uint8_t ret;
            size_t uIndex = uStartBit >> 3;
            size_t uBase = uStartBit - (uIndex << 3);
            if (uBase + uNumBits > 8)
            {
                size_t uFirstIndexBits = 8 - uBase;
                size_t uNextIndexBits = uNumBits - uFirstIndexBits;
                ret = (m_uBits[uIndex] >> uBase) | ((m_uBits[uIndex + 1] & ((1 << uNextIndexBits) - 1)) << uFirstIndexBits);
            }
            else
            {
                ret = (m_uBits[uIndex] >> uBase) & ((1 << uNumBits) - 1);
            }
            assert(ret < (1 << uNumBits));
            uStartBit += uNumBits;
            return ret;
        }

        void SetBit(_Inout_ size_t& uStartBit, _In_ uint8_t uValue)
        {
            assert(uStartBit < 128 && uValue < 2);
            _Analysis_assume_(uStartBit < 128 && uValue < 2);
            size_t uIndex = uStartBit >> 3;
            size_t uBase = uStartBit - (uIndex << 3);
            m_uBits[uIndex] &= ~(1 << uBase);
            m_uBits[uIndex] |= uValue << uBase;
            uStartBit++;
        }

        void SetBits(_Inout_ size_t& uStartBit, _In_ size_t uNumBits, _In_ uint8_t uValue)
        {
            if (uNumBits == 0)
                return;
            assert(uStartBit + uNumBits <= 128 && uNumBits <= 8);
            _Analysis_assume_(uStartBit + uNumBits <= 128 && uNumBits <= 8);
            assert(uValue < (1 << uNumBits));
            size_t uIndex = uStartBit >> 3;
            size_t uBase = uStartBit - (uIndex << 3);
            if (uBase + uNumBits > 8)
            {
                size_t uFirstIndexBits = 8 - uBase;
                size_t uNextIndexBits = uNumBits - uFirstIndexBits;
                m_uBits[uIndex] &= ~(((1 << uFirstIndexBits) - 1) << uBase);
                m_uBits[uIndex] |= uValue << uBase;
                m_uBits[uIndex + 1] &= ~((1 << uNextIndexBits) - 1);
                m_uBits[uIndex + 1] |= uValue >> uFirstIndexBits;
            }
            else
            {
                m_uBits[uIndex] &= ~(((1 << uNumBits) - 1) << uBase);
                m_uBits[uIndex] |= uValue << uBase;
            }
            uStartBit += uNumBits;
        }

    private:
        uint8_t m_uBits[SizeInBytes];
    };

    // BC6H compression (16 bits per texel)
    class D3DX_BC6H : private CBits< 16 >
    {
    public:
        void Decode(_In_ bool bSigned, _Out_writes_(NUM_PIXELS_PER_BLOCK) HDRColorA* pOut) const;
        void Encode(_In_ bool bSigned, _In_reads_(NUM_PIXELS_PER_BLOCK) const HDRColorA* const pIn);

    private:
#pragma warning(push)
#pragma warning(disable : 4480)
        enum EField : uint8_t
        {
            NA, // N/A
            M,  // Mode
            D,  // Shape
            RW,
            RX,
            RY,
            RZ,
            GW,
            GX,
            GY,
            GZ,
            BW,
            BX,
            BY,
            BZ,
        };
#pragma warning(pop)

        struct ModeDescriptor
        {
            EField m_eField;
            uint8_t   m_uBit;
        };

        struct ModeInfo
        {
            uint8_t uMode;
            uint8_t uPartitions;
            bool bTransformed;
            uint8_t uIndexPrec;
            LDRColorA RGBAPrec[BC6H_MAX_REGIONS][2];
        };

#pragma warning(push)
#pragma warning(disable : 4512)
        struct EncodeParams
        {
            float fBestErr;
            const bool bSigned;
            uint8_t uMode;
            uint8_t uShape;
            const HDRColorA* const aHDRPixels;
            INTEndPntPair aUnqEndPts[BC6H_MAX_SHAPES][BC6H_MAX_REGIONS];
            INTColor aIPixels[NUM_PIXELS_PER_BLOCK];

            EncodeParams(const HDRColorA* const aOriginal, bool bSignedFormat) :
                fBestErr(FLT_MAX), bSigned(bSignedFormat), aHDRPixels(aOriginal)
            {
                for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
                {
                    aIPixels[i].Set(aOriginal[i], bSigned);
                }
            }
        };
#pragma warning(pop)

        static int Quantize(_In_ int iValue, _In_ int prec, _In_ bool bSigned);
        static int Unquantize(_In_ int comp, _In_ uint8_t uBitsPerComp, _In_ bool bSigned);
        static int FinishUnquantize(_In_ int comp, _In_ bool bSigned);

        static bool EndPointsFit(_In_ const EncodeParams* pEP, _In_reads_(BC6H_MAX_REGIONS) const INTEndPntPair aEndPts[]);

        void GeneratePaletteQuantized(_In_ const EncodeParams* pEP, _In_ const INTEndPntPair& endPts,
            _Out_writes_(BC6H_MAX_INDICES) INTColor aPalette[]) const;
        float MapColorsQuantized(_In_ const EncodeParams* pEP, _In_reads_(np) const INTColor aColors[], _In_ size_t np, _In_ const INTEndPntPair &endPts) const;
        float PerturbOne(_In_ const EncodeParams* pEP, _In_reads_(np) const INTColor aColors[], _In_ size_t np, _In_ uint8_t ch,
            _In_ const INTEndPntPair& oldEndPts, _Out_ INTEndPntPair& newEndPts, _In_ float fOldErr, _In_ int do_b) const;
        void OptimizeOne(_In_ const EncodeParams* pEP, _In_reads_(np) const INTColor aColors[], _In_ size_t np, _In_ float aOrgErr,
            _In_ const INTEndPntPair &aOrgEndPts, _Out_ INTEndPntPair &aOptEndPts) const;
        void OptimizeEndPoints(_In_ const EncodeParams* pEP, _In_reads_(BC6H_MAX_REGIONS) const float aOrgErr[],
            _In_reads_(BC6H_MAX_REGIONS) const INTEndPntPair aOrgEndPts[],
            _Out_writes_all_(BC6H_MAX_REGIONS) INTEndPntPair aOptEndPts[]) const;
        static void SwapIndices(_In_ const EncodeParams* pEP, _Inout_updates_all_(BC6H_MAX_REGIONS) INTEndPntPair aEndPts[],
            _In_reads_(NUM_PIXELS_PER_BLOCK) size_t aIndices[]);
        void AssignIndices(_In_ const EncodeParams* pEP, _In_reads_(BC6H_MAX_REGIONS) const INTEndPntPair aEndPts[],
            _Out_writes_(NUM_PIXELS_PER_BLOCK) size_t aIndices[],
            _Out_writes_(BC6H_MAX_REGIONS) float aTotErr[]) const;
        void QuantizeEndPts(_In_ const EncodeParams* pEP, _Out_writes_(BC6H_MAX_REGIONS) INTEndPntPair* qQntEndPts) const;
        void EmitBlock(_In_ const EncodeParams* pEP, _In_reads_(BC6H_MAX_REGIONS) const INTEndPntPair aEndPts[],
            _In_reads_(NUM_PIXELS_PER_BLOCK) const size_t aIndices[]);
        void Refine(_Inout_ EncodeParams* pEP);

        static void GeneratePaletteUnquantized(_In_ const EncodeParams* pEP, _In_ size_t uRegion, _Out_writes_(BC6H_MAX_INDICES) INTColor aPalette[]);
        float MapColors(_In_ const EncodeParams* pEP, _In_ size_t uRegion, _In_ size_t np, _In_reads_(np) const size_t* auIndex) const;
        float RoughMSE(_Inout_ EncodeParams* pEP) const;

    private:
        const static ModeDescriptor ms_aDesc[][82];
        const static ModeInfo ms_aInfo[];
        const static int ms_aModeToInfo[];
    };

    // BC67 compression (16b bits per texel)
    class D3DX_BC7 : private CBits< 16 >
    {
    public:
        void Decode(_Out_writes_(NUM_PIXELS_PER_BLOCK) HDRColorA* pOut) const;
        void Encode(DWORD flags, _In_reads_(NUM_PIXELS_PER_BLOCK) const HDRColorA* const pIn);

    private:
        struct ModeInfo
        {
            uint8_t uPartitions;
            uint8_t uPartitionBits;
            uint8_t uPBits;
            uint8_t uRotationBits;
            uint8_t uIndexModeBits;
            uint8_t uIndexPrec;
            uint8_t uIndexPrec2;
            LDRColorA RGBAPrec;
            LDRColorA RGBAPrecWithP;
        };

#pragma warning(push)
#pragma warning(disable : 4512)
        struct EncodeParams
        {
            uint8_t uMode;
            LDREndPntPair aEndPts[BC7_MAX_SHAPES][BC7_MAX_REGIONS];
            LDRColorA aLDRPixels[NUM_PIXELS_PER_BLOCK];
            const HDRColorA* const aHDRPixels;

            EncodeParams(const HDRColorA* const aOriginal) : aHDRPixels(aOriginal) {}
        };
#pragma warning(pop)

        static uint8_t Quantize(_In_ uint8_t comp, _In_ uint8_t uPrec)
        {
            assert(0 < uPrec && uPrec <= 8);
            uint8_t rnd = (uint8_t)std::min<uint16_t>(255, uint16_t(comp) + (1 << (7 - uPrec)));
            return rnd >> (8 - uPrec);
        }

        static LDRColorA Quantize(_In_ const LDRColorA& c, _In_ const LDRColorA& RGBAPrec)
        {
            LDRColorA q;
            q.r = Quantize(c.r, RGBAPrec.r);
            q.g = Quantize(c.g, RGBAPrec.g);
            q.b = Quantize(c.b, RGBAPrec.b);
            if (RGBAPrec.a)
                q.a = Quantize(c.a, RGBAPrec.a);
            else
                q.a = 255;
            return q;
        }

        static uint8_t Unquantize(_In_ uint8_t comp, _In_ size_t uPrec)
        {
            assert(0 < uPrec && uPrec <= 8);
            comp = comp << (8 - uPrec);
            return comp | (comp >> uPrec);
        }

        static LDRColorA Unquantize(_In_ const LDRColorA& c, _In_ const LDRColorA& RGBAPrec)
        {
            LDRColorA q;
            q.r = Unquantize(c.r, RGBAPrec.r);
            q.g = Unquantize(c.g, RGBAPrec.g);
            q.b = Unquantize(c.b, RGBAPrec.b);
            q.a = RGBAPrec.a > 0 ? Unquantize(c.a, RGBAPrec.a) : 255;
            return q;
        }

        void GeneratePaletteQuantized(_In_ const EncodeParams* pEP, _In_ size_t uIndexMode, _In_ const LDREndPntPair& endpts,
            _Out_writes_(BC7_MAX_INDICES) LDRColorA aPalette[]) const;
        float PerturbOne(_In_ const EncodeParams* pEP, _In_reads_(np) const LDRColorA colors[], _In_ size_t np, _In_ size_t uIndexMode,
            _In_ size_t ch, _In_ const LDREndPntPair &old_endpts,
            _Out_ LDREndPntPair &new_endpts, _In_ float old_err, _In_ uint8_t do_b) const;
        void Exhaustive(_In_ const EncodeParams* pEP, _In_reads_(np) const LDRColorA aColors[], _In_ size_t np, _In_ size_t uIndexMode,
            _In_ size_t ch, _Inout_ float& fOrgErr, _Inout_ LDREndPntPair& optEndPt) const;
        void OptimizeOne(_In_ const EncodeParams* pEP, _In_reads_(np) const LDRColorA colors[], _In_ size_t np, _In_ size_t uIndexMode,
            _In_ float orig_err, _In_ const LDREndPntPair &orig_endpts, _Out_ LDREndPntPair &opt_endpts) const;
        void OptimizeEndPoints(_In_ const EncodeParams* pEP, _In_ size_t uShape, _In_ size_t uIndexMode,
            _In_reads_(BC7_MAX_REGIONS) const float orig_err[],
            _In_reads_(BC7_MAX_REGIONS) const LDREndPntPair orig_endpts[],
            _Out_writes_(BC7_MAX_REGIONS) LDREndPntPair opt_endpts[]) const;
        void AssignIndices(_In_ const EncodeParams* pEP, _In_ size_t uShape, _In_ size_t uIndexMode,
            _In_reads_(BC7_MAX_REGIONS) LDREndPntPair endpts[],
            _Out_writes_(NUM_PIXELS_PER_BLOCK) size_t aIndices[], _Out_writes_(NUM_PIXELS_PER_BLOCK) size_t aIndices2[],
            _Out_writes_(BC7_MAX_REGIONS) float afTotErr[]) const;
        void EmitBlock(_In_ const EncodeParams* pEP, _In_ size_t uShape, _In_ size_t uRotation, _In_ size_t uIndexMode,
            _In_reads_(BC7_MAX_REGIONS) const LDREndPntPair aEndPts[],
            _In_reads_(NUM_PIXELS_PER_BLOCK) const size_t aIndex[],
            _In_reads_(NUM_PIXELS_PER_BLOCK) const size_t aIndex2[]);
        float Refine(_In_ const EncodeParams* pEP, _In_ size_t uShape, _In_ size_t uRotation, _In_ size_t uIndexMode);

        float MapColors(_In_ const EncodeParams* pEP, _In_reads_(np) const LDRColorA aColors[], _In_ size_t np, _In_ size_t uIndexMode,
            _In_ const LDREndPntPair& endPts, _In_ float fMinErr) const;
        static float RoughMSE(_Inout_ EncodeParams* pEP, _In_ size_t uShape, _In_ size_t uIndexMode);

    private:
        const static ModeInfo ms_aInfo[];
    };
}

// BC6H Compression
const D3DX_BC6H::ModeDescriptor D3DX_BC6H::ms_aDesc[14][82] =
{
    {   // Mode 1 (0x00) - 10 5 5 5
        { M, 0}, { M, 1}, {GY, 4}, {BY, 4}, {BZ, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 2 (0x01) - 7 6 6 6
        { M, 0}, { M, 1}, {GY, 5}, {GZ, 4}, {GZ, 5}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 3 (0x02) - 11 5 4 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RW,10}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 4 (0x06) - 11 4 5 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GW,10}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,10},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 0},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {GY, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 5 (0x0a) - 11 4 4 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,10},
        {BY, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,10},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BW,10}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {BZ, 1},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {BZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 6 (0x0e) - 9 5 5 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 7 (0x12) - 8 6 5 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {GZ, 4}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BZ, 3}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 8 (0x16) - 8 5 6 5
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {BZ, 0}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {GZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BZ, 1}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 9 (0x1a) - 8 5 5 6
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {BY, 5}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {GZ, 4}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {BZ, 0}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {BZ, 2}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {BZ, 3}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 10 (0x1e) - 6 6 6 6
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {GZ, 4}, {BZ, 0}, {BZ, 1}, {BY, 4}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GY, 5}, {BY, 5}, {BZ, 2}, {GY, 4}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {GZ, 5}, {BZ, 3}, {BZ, 5}, {BZ, 4}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {GY, 0}, {GY, 1}, {GY, 2}, {GY, 3}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GZ, 0}, {GZ, 1}, {GZ, 2}, {GZ, 3}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BY, 0}, {BY, 1}, {BY, 2}, {BY, 3}, {RY, 0}, {RY, 1}, {RY, 2}, {RY, 3}, {RY, 4},
        {RY, 5}, {RZ, 0}, {RZ, 1}, {RZ, 2}, {RZ, 3}, {RZ, 4}, {RZ, 5}, { D, 0}, { D, 1}, { D, 2},
        { D, 3}, { D, 4}, 
    },

    {   // Mode 11 (0x03) - 10 10
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RX, 9}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GX, 9}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BX, 9}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, 
    },

    {   // Mode 12 (0x07) - 11 9
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RX, 8}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GX, 8}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BX, 8}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, 
    },

    {   // Mode 13 (0x0b) - 12 8
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RX, 4},
        {RX, 5}, {RX, 6}, {RX, 7}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GX, 4},
        {GX, 5}, {GX, 6}, {GX, 7}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BX, 4},
        {BX, 5}, {BX, 6}, {BX, 7}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, 
    },

    {   // Mode 14 (0x0f) - 16 4
        { M, 0}, { M, 1}, { M, 2}, { M, 3}, { M, 4}, {RW, 0}, {RW, 1}, {RW, 2}, {RW, 3}, {RW, 4},
        {RW, 5}, {RW, 6}, {RW, 7}, {RW, 8}, {RW, 9}, {GW, 0}, {GW, 1}, {GW, 2}, {GW, 3}, {GW, 4},
        {GW, 5}, {GW, 6}, {GW, 7}, {GW, 8}, {GW, 9}, {BW, 0}, {BW, 1}, {BW, 2}, {BW, 3}, {BW, 4},
        {BW, 5}, {BW, 6}, {BW, 7}, {BW, 8}, {BW, 9}, {RX, 0}, {RX, 1}, {RX, 2}, {RX, 3}, {RW,15},
        {RW,14}, {RW,13}, {RW,12}, {RW,11}, {RW,10}, {GX, 0}, {GX, 1}, {GX, 2}, {GX, 3}, {GW,15},
        {GW,14}, {GW,13}, {GW,12}, {GW,11}, {GW,10}, {BX, 0}, {BX, 1}, {BX, 2}, {BX, 3}, {BW,15},
        {BW,14}, {BW,13}, {BW,12}, {BW,11}, {BW,10}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0}, {NA, 0},
        {NA, 0}, {NA, 0}, 
    },
};

// Mode, Partitions, Transformed, IndexPrec, RGBAPrec
const D3DX_BC6H::ModeInfo D3DX_BC6H::ms_aInfo[] =
{
    {0x00, 1, true,  3, { { LDRColorA(10,10,10,0), LDRColorA( 5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 1
    {0x01, 1, true,  3, { { LDRColorA( 7, 7, 7,0), LDRColorA( 6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 2
    {0x02, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA( 5, 4, 4,0) }, { LDRColorA(5,4,4,0), LDRColorA(5,4,4,0) } } }, // Mode 3
    {0x06, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA( 4, 5, 4,0) }, { LDRColorA(4,5,4,0), LDRColorA(4,5,4,0) } } }, // Mode 4
    {0x0a, 1, true,  3, { { LDRColorA(11,11,11,0), LDRColorA( 4, 4, 5,0) }, { LDRColorA(4,4,5,0), LDRColorA(4,4,5,0) } } }, // Mode 5
    {0x0e, 1, true,  3, { { LDRColorA( 9, 9, 9,0), LDRColorA( 5, 5, 5,0) }, { LDRColorA(5,5,5,0), LDRColorA(5,5,5,0) } } }, // Mode 6
    {0x12, 1, true,  3, { { LDRColorA( 8, 8, 8,0), LDRColorA( 6, 5, 5,0) }, { LDRColorA(6,5,5,0), LDRColorA(6,5,5,0) } } }, // Mode 7
    {0x16, 1, true,  3, { { LDRColorA( 8, 8, 8,0), LDRColorA( 5, 6, 5,0) }, { LDRColorA(5,6,5,0), LDRColorA(5,6,5,0) } } }, // Mode 8
    {0x1a, 1, true,  3, { { LDRColorA( 8, 8, 8,0), LDRColorA( 5, 5, 6,0) }, { LDRColorA(5,5,6,0), LDRColorA(5,5,6,0) } } }, // Mode 9
    {0x1e, 1, false, 3, { { LDRColorA( 6, 6, 6,0), LDRColorA( 6, 6, 6,0) }, { LDRColorA(6,6,6,0), LDRColorA(6,6,6,0) } } }, // Mode 10
    {0x03, 0, false, 4, { { LDRColorA(10,10,10,0), LDRColorA(10,10,10,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 11
    {0x07, 0, true,  4, { { LDRColorA(11,11,11,0), LDRColorA( 9, 9, 9,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 12
    {0x0b, 0, true,  4, { { LDRColorA(12,12,12,0), LDRColorA( 8, 8, 8,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 13
    {0x0f, 0, true,  4, { { LDRColorA(16,16,16,0), LDRColorA( 4, 4, 4,0) }, { LDRColorA(0,0,0,0), LDRColorA(0,0,0,0) } } }, // Mode 14
};

const int D3DX_BC6H::ms_aModeToInfo[] =
{
     0, // Mode 1   - 0x00
     1, // Mode 2   - 0x01
     2, // Mode 3   - 0x02
    10, // Mode 11  - 0x03
    -1, // Invalid  - 0x04
    -1, // Invalid  - 0x05
     3, // Mode 4   - 0x06
    11, // Mode 12  - 0x07
    -1, // Invalid  - 0x08
    -1, // Invalid  - 0x09
     4, // Mode 5   - 0x0a
    12, // Mode 13  - 0x0b
    -1, // Invalid  - 0x0c
    -1, // Invalid  - 0x0d
     5, // Mode 6   - 0x0e
    13, // Mode 14  - 0x0f
    -1, // Invalid  - 0x10
    -1, // Invalid  - 0x11
     6, // Mode 7   - 0x12
    -1, // Reserved - 0x13
    -1, // Invalid  - 0x14
    -1, // Invalid  - 0x15
     7, // Mode 8   - 0x16
    -1, // Reserved - 0x17
    -1, // Invalid  - 0x18
    -1, // Invalid  - 0x19
     8, // Mode 9   - 0x1a
    -1, // Reserved - 0x1b
    -1, // Invalid  - 0x1c
    -1, // Invalid  - 0x1d
     9, // Mode 10  - 0x1e
    -1, // Resreved - 0x1f
};

// BC7 compression: uPartitions, uPartitionBits, uPBits, uRotationBits, uIndexModeBits, uIndexPrec, uIndexPrec2, RGBAPrec, RGBAPrecWithP
const D3DX_BC7::ModeInfo D3DX_BC7::ms_aInfo[] =
{
    {2, 4, 6, 0, 0, 3, 0, LDRColorA(4,4,4,0), LDRColorA(5,5,5,0)},
        // Mode 0: Color only, 3 Subsets, RGBP 4441 (unique P-bit), 3-bit indecies, 16 partitions
    {1, 6, 2, 0, 0, 3, 0, LDRColorA(6,6,6,0), LDRColorA(7,7,7,0)},
        // Mode 1: Color only, 2 Subsets, RGBP 6661 (shared P-bit), 3-bit indecies, 64 partitions
    {2, 6, 0, 0, 0, 2, 0, LDRColorA(5,5,5,0), LDRColorA(5,5,5,0)},
        // Mode 2: Color only, 3 Subsets, RGB 555, 2-bit indecies, 64 partitions
    {1, 6, 4, 0, 0, 2, 0, LDRColorA(7,7,7,0), LDRColorA(8,8,8,0)},
        // Mode 3: Color only, 2 Subsets, RGBP 7771 (unique P-bit), 2-bits indecies, 64 partitions
    {0, 0, 0, 2, 1, 2, 3, LDRColorA(5,5,5,6), LDRColorA(5,5,5,6)},
        // Mode 4: Color w/ Separate Alpha, 1 Subset, RGB 555, A6, 16x2/16x3-bit indices, 2-bit rotation, 1-bit index selector
    {0, 0, 0, 2, 0, 2, 2, LDRColorA(7,7,7,8), LDRColorA(7,7,7,8)},
        // Mode 5: Color w/ Separate Alpha, 1 Subset, RGB 777, A8, 16x2/16x2-bit indices, 2-bit rotation
    {0, 0, 2, 0, 0, 4, 0, LDRColorA(7,7,7,7), LDRColorA(8,8,8,8)},
        // Mode 6: Color+Alpha, 1 Subset, RGBAP 77771 (unique P-bit), 16x4-bit indecies
    {1, 6, 4, 0, 0, 2, 0, LDRColorA(5,5,5,5), LDRColorA(6,6,6,6)}
        // Mode 7: Color+Alpha, 2 Subsets, RGBAP 55551 (unique P-bit), 2-bit indices, 64 partitions
};


namespace
{
    //-------------------------------------------------------------------------------------
    // Helper functions
    //-------------------------------------------------------------------------------------
    inline bool IsFixUpOffset(_In_range_(0, 2) size_t uPartitions, _In_range_(0, 63) size_t uShape, _In_range_(0, 15) size_t uOffset)
    {
        assert(uPartitions < 3 && uShape < 64 && uOffset < 16);
        _Analysis_assume_(uPartitions < 3 && uShape < 64 && uOffset < 16);
        for (size_t p = 0; p <= uPartitions; p++)
        {
            if (uOffset == g_aFixUp[uPartitions][uShape][p])
            {
                return true;
            }
        }
        return false;
    }

    inline void TransformForward(_Inout_updates_all_(BC6H_MAX_REGIONS) INTEndPntPair aEndPts[])
    {
        aEndPts[0].B -= aEndPts[0].A;
        aEndPts[1].A -= aEndPts[0].A;
        aEndPts[1].B -= aEndPts[0].A;
    }

    inline void TransformInverse(_Inout_updates_all_(BC6H_MAX_REGIONS) INTEndPntPair aEndPts[], _In_ const LDRColorA& Prec, _In_ bool bSigned)
    {
        INTColor WrapMask((1 << Prec.r) - 1, (1 << Prec.g) - 1, (1 << Prec.b) - 1);
        aEndPts[0].B += aEndPts[0].A; aEndPts[0].B &= WrapMask;
        aEndPts[1].A += aEndPts[0].A; aEndPts[1].A &= WrapMask;
        aEndPts[1].B += aEndPts[0].A; aEndPts[1].B &= WrapMask;
        if (bSigned)
        {
            aEndPts[0].B.SignExtend(Prec);
            aEndPts[1].A.SignExtend(Prec);
            aEndPts[1].B.SignExtend(Prec);
        }
    }

    inline float Norm(_In_ const INTColor& a, _In_ const INTColor& b)
    {
        float dr = float(a.r) - float(b.r);
        float dg = float(a.g) - float(b.g);
        float db = float(a.b) - float(b.b);
        return dr * dr + dg * dg + db * db;
    }

    // return # of bits needed to store n. handle signed or unsigned cases properly
    inline int NBits(_In_ int n, _In_ bool bIsSigned)
    {
        int nb;
        if (n == 0)
        {
            return 0;	// no bits needed for 0, signed or not
        }
        else if (n > 0)
        {
            for (nb = 0; n; ++nb, n >>= 1);
            return nb + (bIsSigned ? 1 : 0);
        }
        else
        {
            assert(bIsSigned);
            for (nb = 0; n < -1; ++nb, n >>= 1);
            return nb + 1;
        }
    }


    //-------------------------------------------------------------------------------------
    float OptimizeRGB(
        _In_reads_(NUM_PIXELS_PER_BLOCK) const HDRColorA* const pPoints,
        _Out_ HDRColorA* pX,
        _Out_ HDRColorA* pY,
        _In_range_(3, 4) size_t cSteps,
        size_t cPixels,
        _In_reads_(cPixels) const size_t* pIndex)
    {
        float fError = FLT_MAX;
        const float *pC = (3 == cSteps) ? pC3 : pC4;
        const float *pD = (3 == cSteps) ? pD3 : pD4;

        // Find Min and Max points, as starting point
        HDRColorA X(1.0f, 1.0f, 1.0f, 0.0f);
        HDRColorA Y(0.0f, 0.0f, 0.0f, 0.0f);

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            if (pPoints[pIndex[iPoint]].r < X.r) X.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g < X.g) X.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b < X.b) X.b = pPoints[pIndex[iPoint]].b;
            if (pPoints[pIndex[iPoint]].r > Y.r) Y.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g > Y.g) Y.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b > Y.b) Y.b = pPoints[pIndex[iPoint]].b;
        }

        // Diagonal axis
        HDRColorA AB;
        AB.r = Y.r - X.r;
        AB.g = Y.g - X.g;
        AB.b = Y.b - X.b;

        float fAB = AB.r * AB.r + AB.g * AB.g + AB.b * AB.b;

        // Single color block.. no need to root-find
        if (fAB < FLT_MIN)
        {
            pX->r = X.r; pX->g = X.g; pX->b = X.b;
            pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
            return 0.0f;
        }

        // Try all four axis directions, to determine which diagonal best fits data
        float fABInv = 1.0f / fAB;

        HDRColorA Dir;
        Dir.r = AB.r * fABInv;
        Dir.g = AB.g * fABInv;
        Dir.b = AB.b * fABInv;

        HDRColorA Mid;
        Mid.r = (X.r + Y.r) * 0.5f;
        Mid.g = (X.g + Y.g) * 0.5f;
        Mid.b = (X.b + Y.b) * 0.5f;

        float fDir[4];
        fDir[0] = fDir[1] = fDir[2] = fDir[3] = 0.0f;

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            HDRColorA Pt;
            Pt.r = (pPoints[pIndex[iPoint]].r - Mid.r) * Dir.r;
            Pt.g = (pPoints[pIndex[iPoint]].g - Mid.g) * Dir.g;
            Pt.b = (pPoints[pIndex[iPoint]].b - Mid.b) * Dir.b;

            float f;
            f = Pt.r + Pt.g + Pt.b; fDir[0] += f * f;
            f = Pt.r + Pt.g - Pt.b; fDir[1] += f * f;
            f = Pt.r - Pt.g + Pt.b; fDir[2] += f * f;
            f = Pt.r - Pt.g - Pt.b; fDir[3] += f * f;
        }

        float fDirMax = fDir[0];
        size_t  iDirMax = 0;

        for (size_t iDir = 1; iDir < 4; iDir++)
        {
            if (fDir[iDir] > fDirMax)
            {
                fDirMax = fDir[iDir];
                iDirMax = iDir;
            }
        }

        if (iDirMax & 2) std::swap(X.g, Y.g);
        if (iDirMax & 1) std::swap(X.b, Y.b);

        // Two color block.. no need to root-find
        if (fAB < 1.0f / 4096.0f)
        {
            pX->r = X.r; pX->g = X.g; pX->b = X.b;
            pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
            return 0.0f;
        }

        // Use Newton's Method to find local minima of sum-of-squares error.
        float fSteps = (float)(cSteps - 1);

        for (size_t iIteration = 0; iIteration < 8; iIteration++)
        {
            // Calculate new steps
            HDRColorA pSteps[4] = {};

            for (size_t iStep = 0; iStep < cSteps; iStep++)
            {
                pSteps[iStep].r = X.r * pC[iStep] + Y.r * pD[iStep];
                pSteps[iStep].g = X.g * pC[iStep] + Y.g * pD[iStep];
                pSteps[iStep].b = X.b * pC[iStep] + Y.b * pD[iStep];
            }

            // Calculate color direction
            Dir.r = Y.r - X.r;
            Dir.g = Y.g - X.g;
            Dir.b = Y.b - X.b;

            float fLen = (Dir.r * Dir.r + Dir.g * Dir.g + Dir.b * Dir.b);

            if (fLen < (1.0f / 4096.0f))
                break;

            float fScale = fSteps / fLen;

            Dir.r *= fScale;
            Dir.g *= fScale;
            Dir.b *= fScale;

            // Evaluate function, and derivatives
            float d2X = 0.0f, d2Y = 0.0f;
            HDRColorA dX(0.0f, 0.0f, 0.0f, 0.0f), dY(0.0f, 0.0f, 0.0f, 0.0f);

            for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
            {
                float fDot = (pPoints[pIndex[iPoint]].r - X.r) * Dir.r +
                    (pPoints[pIndex[iPoint]].g - X.g) * Dir.g +
                    (pPoints[pIndex[iPoint]].b - X.b) * Dir.b;

                size_t iStep;
                if (fDot <= 0.0f)
                    iStep = 0;
                if (fDot >= fSteps)
                    iStep = cSteps - 1;
                else
                    iStep = size_t(fDot + 0.5f);

                HDRColorA Diff;
                Diff.r = pSteps[iStep].r - pPoints[pIndex[iPoint]].r;
                Diff.g = pSteps[iStep].g - pPoints[pIndex[iPoint]].g;
                Diff.b = pSteps[iStep].b - pPoints[pIndex[iPoint]].b;

                float fC = pC[iStep] * (1.0f / 8.0f);
                float fD = pD[iStep] * (1.0f / 8.0f);

                d2X += fC * pC[iStep];
                dX.r += fC * Diff.r;
                dX.g += fC * Diff.g;
                dX.b += fC * Diff.b;

                d2Y += fD * pD[iStep];
                dY.r += fD * Diff.r;
                dY.g += fD * Diff.g;
                dY.b += fD * Diff.b;
            }

            // Move endpoints
            if (d2X > 0.0f)
            {
                float f = -1.0f / d2X;

                X.r += dX.r * f;
                X.g += dX.g * f;
                X.b += dX.b * f;
            }

            if (d2Y > 0.0f)
            {
                float f = -1.0f / d2Y;

                Y.r += dY.r * f;
                Y.g += dY.g * f;
                Y.b += dY.b * f;
            }

            if ((dX.r * dX.r < fEpsilon) && (dX.g * dX.g < fEpsilon) && (dX.b * dX.b < fEpsilon) &&
                (dY.r * dY.r < fEpsilon) && (dY.g * dY.g < fEpsilon) && (dY.b * dY.b < fEpsilon))
            {
                break;
            }
        }

        pX->r = X.r; pX->g = X.g; pX->b = X.b;
        pY->r = Y.r; pY->g = Y.g; pY->b = Y.b;
        return fError;
    }


    //-------------------------------------------------------------------------------------
    float OptimizeRGBA(
        _In_reads_(NUM_PIXELS_PER_BLOCK) const HDRColorA* const pPoints,
        _Out_ HDRColorA* pX,
        _Out_ HDRColorA* pY,
        _In_range_(3, 4) size_t cSteps,
        size_t cPixels,
        _In_reads_(cPixels) const size_t* pIndex)
    {
        float fError = FLT_MAX;
        const float *pC = (3 == cSteps) ? pC3 : pC4;
        const float *pD = (3 == cSteps) ? pD3 : pD4;

        // Find Min and Max points, as starting point
        HDRColorA X(1.0f, 1.0f, 1.0f, 1.0f);
        HDRColorA Y(0.0f, 0.0f, 0.0f, 0.0f);

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            if (pPoints[pIndex[iPoint]].r < X.r) X.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g < X.g) X.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b < X.b) X.b = pPoints[pIndex[iPoint]].b;
            if (pPoints[pIndex[iPoint]].a < X.a) X.a = pPoints[pIndex[iPoint]].a;
            if (pPoints[pIndex[iPoint]].r > Y.r) Y.r = pPoints[pIndex[iPoint]].r;
            if (pPoints[pIndex[iPoint]].g > Y.g) Y.g = pPoints[pIndex[iPoint]].g;
            if (pPoints[pIndex[iPoint]].b > Y.b) Y.b = pPoints[pIndex[iPoint]].b;
            if (pPoints[pIndex[iPoint]].a > Y.a) Y.a = pPoints[pIndex[iPoint]].a;
        }

        // Diagonal axis
        HDRColorA AB = Y - X;
        float fAB = AB * AB;

        // Single color block.. no need to root-find
        if (fAB < FLT_MIN)
        {
            *pX = X;
            *pY = Y;
            return 0.0f;
        }

        // Try all four axis directions, to determine which diagonal best fits data
        float fABInv = 1.0f / fAB;
        HDRColorA Dir = AB * fABInv;
        HDRColorA Mid = (X + Y) * 0.5f;

        float fDir[8];
        fDir[0] = fDir[1] = fDir[2] = fDir[3] = fDir[4] = fDir[5] = fDir[6] = fDir[7] = 0.0f;

        for (size_t iPoint = 0; iPoint < cPixels; iPoint++)
        {
            HDRColorA Pt;
            Pt.r = (pPoints[pIndex[iPoint]].r - Mid.r) * Dir.r;
            Pt.g = (pPoints[pIndex[iPoint]].g - Mid.g) * Dir.g;
            Pt.b = (pPoints[pIndex[iPoint]].b - Mid.b) * Dir.b;
            Pt.a = (pPoints[pIndex[iPoint]].a - Mid.a) * Dir.a;

            float f;
            f = Pt.r + Pt.g + Pt.b + Pt.a; fDir[0] += f * f;
            f = Pt.r + Pt.g + Pt.b - Pt.a; fDir[1] += f * f;
            f = Pt.r + Pt.g - Pt.b + Pt.a; fDir[2] += f * f;
            f = Pt.r + Pt.g - Pt.b - Pt.a; fDir[3] += f * f;
            f = Pt.r - Pt.g + Pt.b + Pt.a; fDir[4] += f * f;
            f = Pt.r - Pt.g + Pt.b - Pt.a; fDir[5] += f * f;
            f = Pt.r - Pt.g - Pt.b + Pt.a; fDir[6] += f * f;
            f = Pt.r - Pt.g - Pt.b - Pt.a; fDir[7] += f * f;
        }

        float fDirMax = fDir[0];
        size_t  iDirMax = 0;

        for (size_t iDir = 1; iDir < 8; iDir++)
        {
            if (fDir[iDir] > fDirMax)
            {
                fDirMax = fDir[iDir];
                iDirMax = iDir;
            }
        }

        if (iDirMax & 4) std::swap(X.g, Y.g);
        if (iDirMax & 2) std::swap(X.b, Y.b);
        if (iDirMax & 1) std::swap(X.a, Y.a);

        // Two color block.. no need to root-find
        if (fAB < 1.0f / 4096.0f)
        {
            *pX = X;
            *pY = Y;
            return 0.0f;
        }

        // Use Newton's Method to find local minima of sum-of-squares error.
        float fSteps = (float)(cSteps - 1);

        for (size_t iIteration = 0; iIteration < 8 && fError > 0.0f; iIteration++)
        {
            // Calculate new steps
            HDRColorA pSteps[BC7_MAX_INDICES];

            LDRColorA lX, lY;
            lX = (X * 255.0f).ToLDRColorA();
            lY = (Y * 255.0f).ToLDRColorA();

            for (size_t iStep = 0; iStep < cSteps; iStep++)
            {
                pSteps[iStep] = X * pC[iStep] + Y * pD[iStep];
                //LDRColorA::Interpolate(lX, lY, i, i, wcprec, waprec, aSteps[i]);
            }

            // Calculate color direction
            Dir = Y - X;
            float fLen = Dir * Dir;
            if (fLen < (1.0f / 4096.0f))
                break;

            float fScale = fSteps / fLen;
            Dir *= fScale;

            // Evaluate function, and derivatives
            float d2X = 0.0f, d2Y = 0.0f;
            HDRColorA dX(0.0f, 0.0f, 0.0f, 0.0f), dY(0.0f, 0.0f, 0.0f, 0.0f);

            for (size_t iPoint = 0; iPoint < cPixels; ++iPoint)
            {
                float fDot = (pPoints[pIndex[iPoint]] - X) * Dir;
                size_t iStep;
                if (fDot <= 0.0f)
                    iStep = 0;
                if (fDot >= fSteps)
                    iStep = cSteps - 1;
                else
                    iStep = size_t(fDot + 0.5f);

                HDRColorA Diff = pSteps[iStep] - pPoints[pIndex[iPoint]];
                float fC = pC[iStep] * (1.0f / 8.0f);
                float fD = pD[iStep] * (1.0f / 8.0f);

                d2X += fC * pC[iStep];
                dX += Diff * fC;

                d2Y += fD * pD[iStep];
                dY += Diff * fD;
            }

            // Move endpoints
            if (d2X > 0.0f)
            {
                float f = -1.0f / d2X;
                X += dX * f;
            }

            if (d2Y > 0.0f)
            {
                float f = -1.0f / d2Y;
                Y += dY * f;
            }

            if ((dX * dX < fEpsilon) && (dY * dY < fEpsilon))
                break;
        }

        *pX = X;
        *pY = Y;
        return fError;
    }


    //-------------------------------------------------------------------------------------
    float ComputeError(
        _Inout_ const LDRColorA& pixel,
        _In_reads_(1 << uIndexPrec) const LDRColorA aPalette[],
        uint8_t uIndexPrec,
        uint8_t uIndexPrec2,
        _Out_opt_ size_t* pBestIndex = nullptr,
        _Out_opt_ size_t* pBestIndex2 = nullptr)
    {
        const size_t uNumIndices = size_t(1) << uIndexPrec;
        const size_t uNumIndices2 = size_t(1) << uIndexPrec2;
        float fTotalErr = 0;
        float fBestErr = FLT_MAX;

        if (pBestIndex)
            *pBestIndex = 0;
        if (pBestIndex2)
            *pBestIndex2 = 0;

        XMVECTOR vpixel = XMLoadUByte4(reinterpret_cast<const XMUBYTE4*>(&pixel));

        if (uIndexPrec2 == 0)
        {
            for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
            {
                XMVECTOR tpixel = XMLoadUByte4(reinterpret_cast<const XMUBYTE4*>(&aPalette[i]));
                // Compute ErrorMetric
                tpixel = XMVectorSubtract(vpixel, tpixel);
                float fErr = XMVectorGetX(XMVector4Dot(tpixel, tpixel));
                if (fErr > fBestErr)	// error increased, so we're done searching
                    break;
                if (fErr < fBestErr)
                {
                    fBestErr = fErr;
                    if (pBestIndex)
                        *pBestIndex = i;
                }
            }
            fTotalErr += fBestErr;
        }
        else
        {
            for (size_t i = 0; i < uNumIndices && fBestErr > 0; i++)
            {
                XMVECTOR tpixel = XMLoadUByte4(reinterpret_cast<const XMUBYTE4*>(&aPalette[i]));
                // Compute ErrorMetricRGB
                tpixel = XMVectorSubtract(vpixel, tpixel);
                float fErr = XMVectorGetX(XMVector3Dot(tpixel, tpixel));
                if (fErr > fBestErr)	// error increased, so we're done searching
                    break;
                if (fErr < fBestErr)
                {
                    fBestErr = fErr;
                    if (pBestIndex)
                        *pBestIndex = i;
                }
            }
            fTotalErr += fBestErr;
            fBestErr = FLT_MAX;
            for (size_t i = 0; i < uNumIndices2 && fBestErr > 0; i++)
            {
                // Compute ErrorMetricAlpha
                float ea = float(pixel.a) - float(aPalette[i].a);
                float fErr = ea*ea;
                if (fErr > fBestErr)	// error increased, so we're done searching
                    break;
                if (fErr < fBestErr)
                {
                    fBestErr = fErr;
                    if (pBestIndex2)
                        *pBestIndex2 = i;
                }
            }
            fTotalErr += fBestErr;
        }

        return fTotalErr;
    }


    void FillWithErrorColors(_Out_writes_(NUM_PIXELS_PER_BLOCK) HDRColorA* pOut)
    {
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
#ifdef _DEBUG
            // Use Magenta in debug as a highly-visible error color
            pOut[i] = HDRColorA(1.0f, 0.0f, 1.0f, 1.0f);
#else
            // In production use, default to black
            pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);
#endif
        }
    }
}


//-------------------------------------------------------------------------------------
// BC6H Compression
//-------------------------------------------------------------------------------------
_Use_decl_annotations_
void D3DX_BC6H::Decode(bool bSigned, HDRColorA* pOut) const
{
    assert(pOut);

    size_t uStartBit = 0;
    uint8_t uMode = GetBits(uStartBit, 2);
    if (uMode != 0x00 && uMode != 0x01)
    {
        uMode = (GetBits(uStartBit, 3) << 2) | uMode;
    }

    assert(uMode < 32);
    _Analysis_assume_(uMode < 32);

    if (ms_aModeToInfo[uMode] >= 0)
    {
        assert(ms_aModeToInfo[uMode] < ARRAYSIZE(ms_aInfo));
        _Analysis_assume_(ms_aModeToInfo[uMode] < ARRAYSIZE(ms_aInfo));
        const ModeDescriptor* desc = ms_aDesc[ms_aModeToInfo[uMode]];

        assert(ms_aModeToInfo[uMode] < ARRAYSIZE(ms_aDesc));
        _Analysis_assume_(ms_aModeToInfo[uMode] < ARRAYSIZE(ms_aDesc));
        const ModeInfo& info = ms_aInfo[ms_aModeToInfo[uMode]];

        INTEndPntPair aEndPts[BC6H_MAX_REGIONS];
        memset(aEndPts, 0, BC6H_MAX_REGIONS * 2 * sizeof(INTColor));
        uint32_t uShape = 0;

        // Read header
        const size_t uHeaderBits = info.uPartitions > 0 ? 82 : 65;
        while (uStartBit < uHeaderBits)
        {
            size_t uCurBit = uStartBit;
            if (GetBit(uStartBit))
            {
                switch (desc[uCurBit].m_eField)
                {
                case D:  uShape |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RW: aEndPts[0].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RX: aEndPts[0].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RY: aEndPts[1].A.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case RZ: aEndPts[1].B.r |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GW: aEndPts[0].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GX: aEndPts[0].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GY: aEndPts[1].A.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case GZ: aEndPts[1].B.g |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BW: aEndPts[0].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BX: aEndPts[0].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BY: aEndPts[1].A.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                case BZ: aEndPts[1].B.b |= 1 << uint32_t(desc[uCurBit].m_uBit); break;
                default:
                {
#ifdef _DEBUG
                    OutputDebugStringA("BC6H: Invalid header bits encountered during decoding\n");
#endif
                    FillWithErrorColors(pOut);
                    return;
                }
                }
            }
        }

        assert(uShape < 64);
        _Analysis_assume_(uShape < 64);

        // Sign extend necessary end points
        if (bSigned)
        {
            aEndPts[0].A.SignExtend(info.RGBAPrec[0][0]);
        }
        if (bSigned || info.bTransformed)
        {
            assert(info.uPartitions < BC6H_MAX_REGIONS);
            _Analysis_assume_(info.uPartitions < BC6H_MAX_REGIONS);
            for (size_t p = 0; p <= info.uPartitions; ++p)
            {
                if (p != 0)
                {
                    aEndPts[p].A.SignExtend(info.RGBAPrec[p][0]);
                }
                aEndPts[p].B.SignExtend(info.RGBAPrec[p][1]);
            }
        }

        // Inverse transform the end points
        if (info.bTransformed)
        {
            TransformInverse(aEndPts, info.RGBAPrec[0][0], bSigned);
        }

        // Read indices
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            size_t uNumBits = IsFixUpOffset(info.uPartitions, uShape, i) ? info.uIndexPrec - 1 : info.uIndexPrec;
            if (uStartBit + uNumBits > 128)
            {
#ifdef _DEBUG
                OutputDebugStringA("BC6H: Invalid block encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }
            uint8_t uIndex = GetBits(uStartBit, uNumBits);

            if (uIndex >= ((info.uPartitions > 0) ? 8 : 16))
            {
#ifdef _DEBUG
                OutputDebugStringA("BC6H: Invalid index encountered during decoding\n");
#endif
                FillWithErrorColors(pOut);
                return;
            }

            size_t uRegion = g_aPartitionTable[info.uPartitions][uShape][i];
            assert(uRegion < BC6H_MAX_REGIONS);
            _Analysis_assume_(uRegion < BC6H_MAX_REGIONS);

            // Unquantize endpoints and interpolate
            int r1 = Unquantize(aEndPts[uRegion].A.r, info.RGBAPrec[0][0].r, bSigned);
            int g1 = Unquantize(aEndPts[uRegion].A.g, info.RGBAPrec[0][0].g, bSigned);
            int b1 = Unquantize(aEndPts[uRegion].A.b, info.RGBAPrec[0][0].b, bSigned);
            int r2 = Unquantize(aEndPts[uRegion].B.r, info.RGBAPrec[0][0].r, bSigned);
            int g2 = Unquantize(aEndPts[uRegion].B.g, info.RGBAPrec[0][0].g, bSigned);
            int b2 = Unquantize(aEndPts[uRegion].B.b, info.RGBAPrec[0][0].b, bSigned);
            const int* aWeights = info.uPartitions > 0 ? g_aWeights3 : g_aWeights4;
            INTColor fc;
            fc.r = FinishUnquantize((r1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + r2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
            fc.g = FinishUnquantize((g1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + g2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);
            fc.b = FinishUnquantize((b1 * (BC67_WEIGHT_MAX - aWeights[uIndex]) + b2 * aWeights[uIndex] + BC67_WEIGHT_ROUND) >> BC67_WEIGHT_SHIFT, bSigned);

            HALF rgb[3];
            fc.ToF16(rgb, bSigned);

            pOut[i].r = XMConvertHalfToFloat(rgb[0]);
            pOut[i].g = XMConvertHalfToFloat(rgb[1]);
            pOut[i].b = XMConvertHalfToFloat(rgb[2]);
            pOut[i].a = 1.0f;
        }
    }
    else
    {
#ifdef _DEBUG
        const char* warnstr = "BC6H: Invalid mode encountered during decoding\n";
        switch (uMode)
        {
        case 0x13:  warnstr = "BC6H: Reserved mode 10011 encountered during decoding\n"; break;
        case 0x17:  warnstr = "BC6H: Reserved mode 10111 encountered during decoding\n"; break;
        case 0x1B:  warnstr = "BC6H: Reserved mode 11011 encountered during decoding\n"; break;
        case 0x1F:  warnstr = "BC6H: Reserved mode 11111 encountered during decoding\n"; break;
        }
        OutputDebugStringA(warnstr);
#endif
        // Per the BC6H format spec, we must return opaque black
        for (size_t i = 0; i < NUM_PIXELS_PER_BLOCK; ++i)
        {
            pOut[i] = HDRColorA(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}


_Use_decl_annotations_
void D3DX_BC6H::Encode(bool bSigned, const HDRColorA* const pIn)
{
    assert(pIn);

    EncodeParams EP(pIn, bSigned);

    for (EP.uMode = 0; EP.uMode < ARRAYSIZE(ms_aInfo) && EP.fBestErr > 0; ++EP.uMode)
    {
        const uint8_t uShapes = ms_aInfo[EP.uMode].uPartitions ? 32 : 1;
        // Number of rough cases to look at. reasonable values of this are 1, uShapes/4, and uShapes
        // uShapes/4 gets nearly all the cases; you can increase that a bit (say by 3 or 4) if you really want to squeeze the last bit out
        const size_t uItems = std::max<size_t>(1, uShapes >> 2);
        float afRoughMSE[BC6H_MAX_SHAPES];
        uint8_t auShape[BC6H_MAX_SHAPES];

        // pick the best uItems shapes and refine these.
        for (EP.uShape = 0; EP.uShape < uShapes; ++EP.uShape)
        {
            size_t uShape = EP.uShape;
            afRoughMSE[uShape] = RoughMSE(&EP);
            auShape[uShape] = static_cast<uint8_t>(uShape);
        }

        // Bubble up the first uItems items
        for (size_t i = 0; i < uItems; i++)
        {
            for (size_t j = i + 1; j < uShapes; j++)
            {
                if (afRoughMSE[i] > afRoughMSE[j])
                {
                    std::swap(afRoughMSE[i], afRoughMSE[j]);
                    std::swap(auShape[i], auShape[j]);
                }
            }
        }

        for (size_t i = 0; i < uItems && EP.fBestErr > 0; i++)
        {
            EP.uShape = auShape[i];
            Refine(&EP);
        }
    }
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
int D3DX_BC6H::Quantize(int iValue, int prec, bool bSigned)
{
    assert(prec > 1);	// didn't bother to make it work for 1
    int q, s = 0;
    if (bSigned)
    {
        assert(iValue >= -F16MAX && iValue <= F16MAX);
        if (iValue < 0)
        {
            s = 1;
            iValue = -iValue;
        }
        q = (prec >= 16) ? iValue : (iValue << (prec - 1)) / (F16MAX + 1);
        if (s)
            q = -q;
        assert(q > -(1 << (prec - 1)) && q < (1 << (prec - 1)));
    }
    else
    {
        assert(iValue >= 0 && iValue <= F16MAX);
        q = (prec >= 15) ? iValue : (iValue << prec) / (F16MAX + 1);
        assert(q >= 0 && q < (1 << prec));
    }

    return q;
}


_Use_decl_annotations_
int D3DX_BC6H::Unquantize(int comp, uint8_t uBitsPerComp, bool bSigned)
{
    int unq = 0, s = 0;
    if (bSigned)
    {
        if (uBitsPerComp >= 16)
        {
            unq = comp;
        }
        else
        {
            if (comp < 0)
            {
                s = 1;
                comp = -comp;
            }

            if (comp == 0) unq = 0;
            else if (comp >= ((1 << (uBitsPerComp - 1)) - 1)) unq = 0x7FFF;
            else unq = ((comp << 15) + 0x4000) >> (uBitsPerComp - 1);

            if (s) unq = -unq;
        }
    }
    else
    {
        if (uBitsPerComp >= 15) unq = comp;
        else if (comp == 0) unq = 0;
        else if (comp == ((1 << uBitsPerComp) - 1)) unq = 0xFFFF;
        else unq = ((comp << 16) + 0x8000) >> uBitsPerComp;
    }

    return unq;
}


_Use_decl_annotations_
int D3DX_BC6H::FinishUnquantize(int comp, bool bSigned)
{
    if (bSigned)
    {
        return (comp < 0) ? -(((-comp) * 31) >> 5) : (comp * 31) >> 5;  // scale the magnitude by 31/32
    }
    else
    {
        return (comp * 31) >> 6;                                        // scale the magnitude by 31/64
    }
}


//-------------------------------------------------------------------------------------
_Use_decl_annotations_
bool D3DX_BC6H::EndPointsFit(const EncodeParams* pEP, const INTEndPntPair aEndPts[])
{
    assert(pEP);
    const bool bTransformed = ms_aInfo[pEP->uMode].bTransformed;
    const bool bIsSigned = pEP->bSigned;
    const LDRColorA& Prec0 = ms_aInfo[pEP->uMode].RGBAPrec[0][0];
    const LDRColorA& Prec1 = ms_aInfo[pEP->uMode].RGBAPrec[0][1];
    const LDRColorA& Prec2 = ms_aInfo[pEP->uMode].RGBAPrec[1][0];
    const LDRColorA& Prec3 = ms_aInfo[pEP->uMode].RGBAPrec[1][1];

    INTColor aBits[4];
    aBits[0].r = NBits(aEndPts[0].A.r, bIsSigned);
    aBits[0].g = NBits(aEndPts[0].A.g, bIsSigned);
    aBits[0].b = NBits(aEndPts[0].A.b, bIsSigned);
    aBits[1].r = NBits(aEndPts[0].B.r, bTransformed || bIsSigned);
    aBits[1].g = NBits(aEndPts[0].B.g, bTransformed || bIsSigned);
    aBits[1].b = NBits(aEndPts[0].B.b, bTransformed || bIsSigned);
    if (aBits[0].r > Prec0.r || aBits[1].r > Prec1.r ||
        aBits[0].g > Prec0.g || aBits[1].g > Prec1.g ||
        aBits[0].b > Prec0.b || aBits[1].b > Prec1.b)
        return false;

    if (ms_aInfo[pEP->uMode].uPartitions)
    {
        aBits[2].r = NBits(aEndPts[1].A.r, bTransformed || bIsSigned);
        aBits[2].g = NBits(aEndPts[1].A.g, bTransformed || bIsSigned);
        aBits[2].b = NBits(aEndPts[1].A.b, bTransformed || bIsSigned);
        aBits[3].r = NBits(aEndPts[1].B.r, bTransformed || bIsSigned);
        aBits[3].g = NBits(aEndPts[1].B.g, bTransformed || bIsSigned);
        aBits[3].b = NBits(aEndPts[1].B.b, bTransformed || bIsSigned);

        if (aBits[2].r > Prec2.r || aBits[3].r > Prec3.r ||
            aBits[2].g > Prec2.g || aBits[3].g > Prec3.g ||
            aBits[2].b > Prec2.b || aBits[3].b > Prec3.b)
            return false;
    }

    return true;
}


