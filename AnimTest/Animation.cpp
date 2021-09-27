//--------------------------------------------------------------------------------------
// File: Animation.cpp
//
// Simple animation playback system for SDKMESH and CMO
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Animation.h"

#include <fstream>

using namespace DX;

namespace
{
#pragma pack(push,8)

    static constexpr uint32_t MAX_FRAME_NAME = 100;

    struct SDKANIMATION_FILE_HEADER
    {
        uint32_t Version;
        uint8_t  IsBigEndian;
        uint32_t FrameTransformType;
        uint32_t NumFrames;
        uint32_t NumAnimationKeys;
        uint32_t AnimationFPS;
        uint64_t AnimationDataSize;
        uint64_t AnimationDataOffset;
    };

    struct SDKANIMATION_DATA
    {
        DirectX::XMFLOAT3 Translation;
        DirectX::XMFLOAT4 Orientation;
        DirectX::XMFLOAT3 Scaling;
    };

    struct SDKANIMATION_FRAME_DATA
    {
        char FrameName[MAX_FRAME_NAME];
        uint64_t DataOffset;
    };

#pragma pack(pop)
}

AnimationSDKMESH::AnimationSDKMESH() noexcept :
    m_animSize(0)
{
}

HRESULT AnimationSDKMESH::Load(_In_z_ const wchar_t* fileName)
{
    Release();

    if (!fileName)
        return E_INVALIDARG;

    std::ifstream inFile(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (!inFile)
        return E_FAIL;

    std::streampos len = inFile.tellg();
    if (!inFile)
        return E_FAIL;

    if (len < sizeof(SDKANIMATION_FILE_HEADER))
        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

    if (len > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_FILE_TOO_LARGE);

    std::unique_ptr<uint8_t[]> blob(new (std::nothrow) uint8_t[size_t(len)]);
    if (!blob)
        return E_OUTOFMEMORY;

    inFile.seekg(0, std::ios::beg);
    if (!inFile)
        return E_FAIL;

    inFile.read(reinterpret_cast<char*>(blob.get()), len);
    if (!inFile)
        return E_FAIL;

    inFile.close();

    auto header = reinterpret_cast<const SDKANIMATION_FILE_HEADER*>(blob.get());

    if (header->Version != 101 /*SDKMESH_FILE_VERSION*/
        || header->IsBigEndian != 0
        || header->FrameTransformType != 0 /*FTT_RELATIVE*/
        || header->NumAnimationKeys == 0
        || header->NumFrames == 0
        || header->AnimationFPS <= 0.f)
        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

    uint64_t dataSize = header->AnimationDataOffset + header->AnimationDataSize;
    if (dataSize > uint64_t(len))
        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

    m_animData.swap(blob);
    m_animSize = static_cast<size_t>(len);

    return S_OK;
}
