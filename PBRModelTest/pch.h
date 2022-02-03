//--------------------------------------------------------------------------------------
// File: pch.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

// Use the C++ standard templated min/max
#define NOMINMAX

#include <winapifamily.h>

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <xdk.h>
#elif !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP) 
#include <WinSDKVer.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <SDKDDKVer.h>

#pragma warning(push)
#pragma warning(disable : 4005)

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#pragma warning(pop)

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <Windows.h>
#endif

#include <wrl/client.h>

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP) 
#include <d3d11_3.h>
#include <dxgi1_6.h>
#else
#include <d3d11_1.h>
#include <dxgi1_6.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#endif

#define _XM_NO_XMVECTOR_OVERLOADS_
#include <DirectXMath.h>
#include <DirectXColors.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cwchar>
#include <exception>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <tuple>

#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "GamePad.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PostProcess.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"

namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
#ifdef _DEBUG
            char str[64] = {};
            sprintf_s(str, "**ERROR** Fatal Error with HRESULT of %08X\n", static_cast<unsigned int>(hr));
            OutputDebugStringA(str);
            __debugbreak();
#endif
            throw com_exception(hr);
        }
    }
}

// Adapters for older DirectXMath used by Xbox One XDK
#if DIRECTX_MATH_VERSION < 313
#pragma warning(push)
#pragma warning(disable : 4201)

namespace DirectX
{
    struct XMFLOAT3X4
    {
        union
        {
            struct
            {
                float _11, _12, _13, _14;
                float _21, _22, _23, _24;
                float _31, _32, _33, _34;
            };
            float m[3][4];
            float f[12];
        };

        XMFLOAT3X4() = default;

        XMFLOAT3X4(const XMFLOAT3X4&) = default;
        XMFLOAT3X4& operator=(const XMFLOAT3X4&) = default;

        XMFLOAT3X4(XMFLOAT3X4&&) = default;
        XMFLOAT3X4& operator=(XMFLOAT3X4&&) = default;

        constexpr XMFLOAT3X4(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23) noexcept
            : _11(m00), _12(m01), _13(m02), _14(m03),
            _21(m10), _22(m11), _23(m12), _24(m13),
            _31(m20), _32(m21), _33(m22), _34(m23) {}
        explicit XMFLOAT3X4(_In_reads_(12) const float* pArray) noexcept
        {
            assert(pArray != nullptr);

            m[0][0] = pArray[0];
            m[0][1] = pArray[1];
            m[0][2] = pArray[2];
            m[0][3] = pArray[3];

            m[1][0] = pArray[4];
            m[1][1] = pArray[5];
            m[1][2] = pArray[6];
            m[1][3] = pArray[7];

            m[2][0] = pArray[8];
            m[2][1] = pArray[9];
            m[2][2] = pArray[10];
            m[2][3] = pArray[11];
        }

        float operator() (size_t Row, size_t Column) const noexcept { return m[Row][Column]; }
        float& operator() (size_t Row, size_t Column) noexcept { return m[Row][Column]; }
    };

    inline void XM_CALLCONV XMStoreFloat3x4(_Out_ XMFLOAT3X4* pDestination, _In_ FXMMATRIX M) noexcept
    {
        XMFLOAT4X4A m;
        XMStoreFloat4x4A(&m, M);

        pDestination->m[0][0] = m._11;
        pDestination->m[0][1] = m._21;
        pDestination->m[0][2] = m._31;
        pDestination->m[0][3] = m._41;

        pDestination->m[1][0] = m._12;
        pDestination->m[1][1] = m._22;
        pDestination->m[1][2] = m._32;
        pDestination->m[1][3] = m._42;

        pDestination->m[2][0] = m._13;
        pDestination->m[2][1] = m._23;
        pDestination->m[2][2] = m._33;
        pDestination->m[2][3] = m._43;
    }
}

#pragma warning(pop)
#endif // DIRECTX_MATH_VERSION

// Enable off by default warnings to improve code conformance
#pragma warning(default : 4061 4062 4191 4242 4263 4264 4265 4266 4289 4302 4365 4746 4826 4841 4987 5029 5038 5042)
