//--------------------------------------------------------------------------------------
// File: Animation.h
//
// Simple animation playback system for SDKMESH for DirectX Tool Kit
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
#pragma once

#include <DirectXMath.h>
#include <Model.h>

#include <memory>
#include <vector>


namespace DX
{
    class AnimationSDKMESH
    {
    public:
        AnimationSDKMESH() noexcept;
        ~AnimationSDKMESH() = default;

        AnimationSDKMESH(AnimationSDKMESH&&) = default;
        AnimationSDKMESH& operator= (AnimationSDKMESH&&) = default;

        AnimationSDKMESH(AnimationSDKMESH const&) = delete;
        AnimationSDKMESH& operator= (AnimationSDKMESH const&) = delete;

        HRESULT Load(_In_z_ const wchar_t* fileName);

        void Release()
        {
            m_animData.reset();
            m_animSize = 0;
            m_boneToTrack.clear();
            m_animBones.reset();
        }

        bool Bind(const DirectX::Model& model);

        void Apply(
            const DirectX::Model& model,
            double time,
            size_t nbones,
            _Out_writes_(nbones) DirectX::XMMATRIX* boneTransforms) const;

    private:
        std::unique_ptr<uint8_t[]>          m_animData;
        size_t                              m_animSize;
        std::vector<uint32_t>               m_boneToTrack;
        DirectX::ModelBone::TransformArray  m_animBones;
    };
}
