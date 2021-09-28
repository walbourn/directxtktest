//--------------------------------------------------------------------------------------
// File: Animation.cpp
//
// Simple animation playback system for SDKMESH for DirectX Tool Kit
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Animation.h"

#include <cassert>
#include <fstream>
#include <stdexcept>

using namespace DX;
using namespace DirectX;

namespace
{
#pragma pack(push,8)

    static constexpr uint32_t SDKMESH_FILE_VERSION = 101;
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

    static_assert(sizeof(SDKANIMATION_FILE_HEADER) == 40, "SDK Mesh structure size incorrect");

    struct SDKANIMATION_DATA
    {
        XMFLOAT3 Translation;
        XMFLOAT4 Orientation;
        XMFLOAT3 Scaling;
    };

    static_assert(sizeof(SDKANIMATION_DATA) == 40, "SDK Mesh structure size incorrect");

    struct SDKANIMATION_FRAME_DATA
    {
        char FrameName[MAX_FRAME_NAME];
        union
        {
            uint64_t DataOffset;
            SDKANIMATION_DATA* pAnimationData;
        };
    };

    static_assert(sizeof(SDKANIMATION_FRAME_DATA) == 112, "SDK Mesh structure size incorrect");

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

    if (header->Version != SDKMESH_FILE_VERSION
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

bool AnimationSDKMESH::Bind(const Model& model)
{
    assert(m_animData && m_animSize > 0);

    if (model.bones.empty())
        return false;

    auto header = reinterpret_cast<const SDKANIMATION_FILE_HEADER*>(m_animData.get());
    assert(header->Version == SDKMESH_FILE_VERSION);
    auto frameData = reinterpret_cast<SDKANIMATION_FRAME_DATA*>(m_animData.get() + header->AnimationDataOffset);

    m_boneToTrack.resize(model.bones.size());
    for (auto& it : m_boneToTrack)
    {
        it = ModelBone::c_Invalid;
    }

    bool result = false;

    for (size_t j = 0; j < header->NumFrames; ++j)
    {
        uint64_t offset = sizeof(SDKANIMATION_FILE_HEADER) + frameData[j].DataOffset;
        uint64_t end = offset + sizeof(SDKANIMATION_DATA) * uint64_t(header->NumAnimationKeys);
        if (end > UINT32_MAX
            || end > m_animSize)
            throw std::runtime_error("Animation file invalid");

        frameData[j].pAnimationData = reinterpret_cast<SDKANIMATION_DATA*>(m_animData.get() + offset);

        wchar_t frameName[MAX_FRAME_NAME] = {};
        MultiByteToWideChar(CP_UTF8, 0, frameData[j].FrameName, -1, frameName, MAX_FRAME_NAME);

        size_t count = 0;
        for (const auto it : model.bones)
        {
            if (_wcsicmp(frameName, it.name.c_str()) == 0)
            {
                m_boneToTrack[count] = static_cast<uint32_t>(j);
                result = true;
                break;
            }

            ++count;
        }
    }

    m_animBones = ModelBone::MakeArray(model.bones.size());

    return result;
}

void AnimationSDKMESH::Apply(
    const DirectX::Model& model,
    double time,
    size_t nbones,
    XMMATRIX* boneTransforms) const
{
    assert(m_animData && m_animSize > 0);

    if (!nbones || !boneTransforms)
    {
        throw std::invalid_argument("Bone transforms array required");
    }

    if (nbones < model.bones.size())
    {
        throw std::invalid_argument("Bone transforms array is too small");
    }

    if (model.bones.empty())
    {
        throw std::runtime_error("Model is missing bones");
    }

    auto header = reinterpret_cast<const SDKANIMATION_FILE_HEADER*>(m_animData.get());
    assert(header->Version == SDKMESH_FILE_VERSION);

    // Determine animation time
    auto tick = static_cast<uint32_t>(header->AnimationFPS * time);
    tick %= header->NumAnimationKeys;

    // Compute local bone transforms
    auto frameData = reinterpret_cast<SDKANIMATION_FRAME_DATA*>(m_animData.get() + header->AnimationDataOffset);

    for (size_t j = 0; j < nbones; ++j)
    {
        if (m_boneToTrack[j] == ModelBone::c_Invalid)
        {
            m_animBones[j] = model.boneMatrices[j];
        }
        else
        {
            auto frame = &frameData[m_boneToTrack[j]];
            auto data = &frame->pAnimationData[tick];

            XMVECTOR quat = XMVectorSet(data->Orientation.x, data->Orientation.y, data->Orientation.z, data->Orientation.w);
            if (XMVector4Equal(quat, g_XMZero))
                quat = XMQuaternionIdentity();
            else
                quat = XMQuaternionNormalize(quat);

            XMMATRIX trans = XMMatrixTranslation(data->Translation.x, data->Translation.y, data->Translation.z);
            XMMATRIX rotation = XMMatrixRotationQuaternion(quat);
            XMMATRIX scale = XMMatrixScaling(data->Scaling.x, data->Scaling.y, data->Scaling.z);

            m_animBones[j] = XMMatrixMultiply(XMMatrixMultiply(rotation, scale), trans);
        }
    }

    // Compute absolute locations
    model.CopyAbsoluteBoneTransforms(nbones, m_animBones.get(), boneTransforms);

    // Adjust for model's bind pose.
    for (size_t j = 0; j < nbones; ++j)
    {
        boneTransforms[j] = XMMatrixMultiply(model.invBindPoseMatrices[j], boneTransforms[j]);
    }
}
