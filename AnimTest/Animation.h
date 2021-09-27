//--------------------------------------------------------------------------------------
// File: Animation.h
//
// Simple animation playback system for SDKMESH and CMO
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
#pragma once

#include <DirectXMath.h>
#include <Model.h>

#include <memory>


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
        }

    private:
        std::unique_ptr<uint8_t[]>  m_animData;
        size_t                      m_animSize;
    };
}
