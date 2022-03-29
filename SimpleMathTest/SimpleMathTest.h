//-------------------------------------------------------------------------------------
// SimpleMathTest.h
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <DirectXColors.h>

#include <stdio.h>

#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

constexpr float EPSILON = 0.000001f;
constexpr float EPSILON2 = 0.00001f;
constexpr float EPSILON3 = 0.001f;

XMGLOBALCONST DirectX::XMVECTORF32 VEPSILON = { { { EPSILON, EPSILON, EPSILON, EPSILON } } };
XMGLOBALCONST DirectX::XMVECTORF32 VEPSILON2 = { { { EPSILON2, EPSILON2, EPSILON2, EPSILON2 } } };
XMGLOBALCONST DirectX::XMVECTORF32 VEPSILON3 = { { { EPSILON3, EPSILON3, EPSILON3, EPSILON3 } } };

inline void FormatValue(bool value, char* output, size_t outputSize)
{
    strcpy_s(output, outputSize, value ? "true" : "false");
}

inline void FormatValue(float value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "%f", value);
}

inline void FormatValue(uint32_t value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "%08x", value);
}

inline void FormatValue(DirectX::SimpleMath::Vector3 const& value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "%f %f %f", value.x, value.y, value.z);
}

inline void FormatValue(DirectX::SimpleMath::Vector4 const& value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w);
}

inline void FormatValue(DirectX::SimpleMath::Color const& value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w);
}

inline void FormatValue(DirectX::SimpleMath::Plane const& value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w);
}

inline void FormatValue(DirectX::SimpleMath::Quaternion const& value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "%f %f %f %f", value.x, value.y, value.z, value.w);
}

inline void FormatValue(DirectX::SimpleMath::Matrix const& value, char* output, size_t outputSize)
{
    sprintf_s(output, outputSize, "\n    %f %f %f %f\n    %f %f %f %f\n    %f %f %f %f\n    %f %f %f %f\n", value._11, value._12, value._13, value._14, value._21, value._22, value._23, value._24, value._31, value._32, value._33, value._34, value._41, value._42, value._43, value._44);
}

struct near_equal_to
{
    bool operator() (float a, float b) const
    {
        return DirectX::XMScalarNearEqual(a, b, EPSILON);
    }

    bool operator() (DirectX::FXMVECTOR a, DirectX::FXMVECTOR b) const
    {
        return DirectX::XMVector4NearEqual(a, b, VEPSILON);
    }

    bool operator() (DirectX::CXMMATRIX a, DirectX::CXMMATRIX b) const
    {
        return DirectX::XMVector4NearEqual(a.r[0], b.r[0], VEPSILON) &&
            DirectX::XMVector4NearEqual(a.r[1], b.r[1], VEPSILON) &&
            DirectX::XMVector4NearEqual(a.r[2], b.r[2], VEPSILON) &&
            DirectX::XMVector4NearEqual(a.r[3], b.r[3], VEPSILON);
    }
};


template<typename TValue, typename TCompare>
inline bool VerifyValue(TValue const& value, TValue const& expected, TCompare const& compare, char const* file, int line)
{
    if (compare(value, expected))
        return true;

    char valueString[256] = {};
    char expectedString[256] = {};

    FormatValue(value, valueString, sizeof(valueString));
    FormatValue(expected, expectedString, sizeof(expectedString));

    printf("ERROR: %s:%d: %s (expecting %s)\n", file, line, valueString, expectedString);

    return false;
}


#define VerifyEqual(value, expected) \
    success &= VerifyValue(value, expected, std::equal_to<decltype(value)>(), __FUNCTION__, __LINE__)

#define VerifyNearEqual(value, expected) \
    success &= VerifyValue(value, expected, near_equal_to(), __FUNCTION__, __LINE__)
