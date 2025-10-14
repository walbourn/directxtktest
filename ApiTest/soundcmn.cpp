//--------------------------------------------------------------------------------------
// File: soundcmn.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#include "SoundCommon.h"

#include <d3d11.h>

#include <cstdio>
#include <iterator>

using namespace DirectX;

_Success_(return)
bool TestA02(_In_ ID3D11Device *device)
{
    if (!device)
        return false;

    bool success = true;

    // IsValid
    struct TestFormats
    {
        WAVEFORMATEX wfx;
        uint32_t testCase;
    };

    static const TestFormats s_invalidwfx[] =
    {
        // { wFormatTag | nChannels | nSamplesPerSec | nAvgBytesPerSec | nBlockAlign | wBitsPerSample | cbSize } | subcase
        { { 0, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0, 2, 44100, 0, 0, 16, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 0, 0, 0, 0, 0, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 1024, 0, 0, 0, 0, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 1, 0, 0, 0, 0, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 1, 2000000, 0, 0, 0, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 2, 44100, 0, 0, 23, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 2, 44100, 0, 0, 16, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 2, 44100, 88200, 4, 16, 0 }, 1 },
        { { WAVE_FORMAT_PCM, 2, 44100, 88200, 4, 16, 0 }, 2 },
        { { WAVE_FORMAT_PCM, 2, 44100, 88200, 4, 16, 0 }, 3 },
        { { WAVE_FORMAT_PCM, 2, 44100, 88200, 4, 16, 0 }, 4 },
        { { WAVE_FORMAT_PCM, 2, 44100, 88200, 4, 16, 0 }, 5 },
        { { WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0 }, 6 },
        { { WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0 }, 7 },
        { { WAVE_FORMAT_PCM, 2, 44100, 0, 4, 20, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 2, 44100, 44100, 0, 16, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 2, 44100, 44100, 1, 16, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 2, 44100, 88200, 3, 16, 0 }, 0 },
        { { WAVE_FORMAT_PCM, 2, 44100, 88200, 4, 16, 0 }, 0 },
        { { WAVE_FORMAT_ADPCM, 3, 44100, 88200, 4, 16, 0 }, 0 },
        { { WAVE_FORMAT_ADPCM, 2, 44100, 88200, 4, 16, 0 }, 0 },
        { { WAVE_FORMAT_ADPCM, 2, 44100, 88200, 4, 4, 0 }, 0 },
        { { WAVE_FORMAT_ADPCM, 2, 44100, 88200, 4, 4, 32 }, 0 },
        { { WAVE_FORMAT_ADPCM, 2, 44100, 88200, 4, 4, 32 }, 1 },
        { { WAVE_FORMAT_ADPCM, 2, 44100, 88200, 4, 4, 32 }, 2 },
        { { WAVE_FORMAT_ADPCM, 2, 44100, 88200, 4, 4, 32 }, 3 },
        { { WAVE_FORMAT_ADPCM, 1, 44100, 88200, 4, 4, 32 }, 4 },
        { { WAVE_FORMAT_ADPCM, 2, 44100, 88200, 4, 4, 32 }, 4 },
        { { WAVE_FORMAT_IEEE_FLOAT, 2, 44100, 88200, 4, 0, 0 }, 0 },
        { { WAVE_FORMAT_IEEE_FLOAT, 2, 44100, 88200, 4, 32, 0 }, 0 },
        { { WAVE_FORMAT_IEEE_FLOAT, 1, 44100, 88200, 4, 32, 0 }, 0 },
        { { WAVE_FORMAT_IEEE_FLOAT, 2, 44100, 88200, 4, 32, 0 }, 1 },
        { { WAVE_FORMAT_WMAUDIO2, 2, 44100, 0, 0, 8, 0 }, 0 },
        { { WAVE_FORMAT_WMAUDIO2, 2, 44100, 0, 0, 16, 0 }, 0 },
        { { WAVE_FORMAT_WMAUDIO2, 2, 44100, 0, 4, 16, 0 }, 0 },
        { { WAVE_FORMAT_WMAUDIO3, 2, 44100, 0, 0, 8, 0 }, 0 },
        { { WAVE_FORMAT_WMAUDIO3, 2, 44100, 0, 0, 16, 0 }, 0 },
        { { WAVE_FORMAT_WMAUDIO3, 2, 44100, 0, 4, 16, 0 }, 0 },
        { { 0x166 /* WAVE_FORMAT_XMA2 */, 0, 0, 0, 0, 0, 0 }, 0 },
        { { 0x166 /* WAVE_FORMAT_XMA2 */, 2, 44100, 88200, 4, 16, 0 }, 0 },
    };

    for(size_t j=0; j < std::size(s_invalidwfx); ++j)
    {
        char buff[64] = {};
        auto invalid = reinterpret_cast<WAVEFORMATEX*>(buff);
        *invalid = s_invalidwfx[j].wfx;

        WAVEFORMATEXTENSIBLE invalidExt = {
            *invalid /* Format */,
            {}, /* wReserved */
            0u, /* dwChannelMask */
            { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } } /* SubFormat */
        };

        invalidExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        invalidExt.SubFormat.Data1 = invalid->wFormatTag;
        invalidExt.Format.cbSize = 22;

        switch(invalid->wFormatTag)
        {
            case WAVE_FORMAT_PCM:
                switch(s_invalidwfx[j].testCase)
                {
                case 1:
                    invalidExt.SubFormat = {};
                    break;
                case 2:
                    invalidExt.Samples.wValidBitsPerSample = 32;
                    break;
                case 3:
                    invalidExt.Samples.wValidBitsPerSample = 31;
                    break;
                case 4:
                    invalidExt.SubFormat.Data1 = 0;
                    break;
                case 5:
                    invalidExt.dwChannelMask = 0xffff; // too many channels
                    break;
                case 6:
                    invalid->nAvgBytesPerSec = 0;
                    invalidExt.dwChannelMask = 0xffff; // too many channels
                    break;
                case 7:
                    invalid->nAvgBytesPerSec = 0;
                    invalidExt.Format.cbSize = 0;
                    break;
                default:
                    break;
                }
                break;

            case WAVE_FORMAT_IEEE_FLOAT:
                switch(s_invalidwfx[j].testCase)
                {
                case 1:
                    invalidExt.Samples.wValidBitsPerSample = 24;
                    break;
                default:
                    break;
                }
                break;

        case WAVE_FORMAT_ADPCM:
            {
                auto wfadpcm = reinterpret_cast<ADPCMWAVEFORMAT*>(buff);
                switch(s_invalidwfx[j].testCase)
                {
                case 1: // Invalid num coeffs
                    wfadpcm->wNumCoef  = 47;
                    break;
                case 2: // Invalid coeffs
                    wfadpcm->wNumCoef  = 7;
                    break;
                case 3: // Invalid block size
                    wfadpcm->wNumCoef  = 7;
                    wfadpcm->aCoef[0] =  { 256, 0 };
                    wfadpcm->aCoef[1] =  { 512, -256 };
                    wfadpcm->aCoef[2] =  { 0, 0 };
                    wfadpcm->aCoef[3] =  { 192, 64 };
                    wfadpcm->aCoef[4] =  { 240, 0 };
                    wfadpcm->aCoef[5] =  { 460, -208 };
                    wfadpcm->aCoef[6] =  { 392, -232 };
                    wfadpcm->wSamplesPerBlock = 0;
                    break;
                case 4: // mono non-even samples
                    wfadpcm->wNumCoef  = 7;
                    wfadpcm->aCoef[0] =  { 256, 0 };
                    wfadpcm->aCoef[1] =  { 512, -256 };
                    wfadpcm->aCoef[2] =  { 0, 0 };
                    wfadpcm->aCoef[3] =  { 192, 64 };
                    wfadpcm->aCoef[4] =  { 240, 0 };
                    wfadpcm->aCoef[5] =  { 460, -208 };
                    wfadpcm->aCoef[6] =  { 392, -232 };
                    wfadpcm->wSamplesPerBlock = 7;
                    break;
                default:
                    break;
                }
            }
            break;

        default:
            break;
        }

        if (IsValid(invalid))
        {
            printf("ERROR: Failed invalid wave format test (%zu)\n", j);
            success = false;
        }

        if (IsValid(reinterpret_cast<WAVEFORMATEX*>(&invalidExt)))
        {
            printf("ERROR: Failed invalid wave extensible format test (%zu)\n", j);
            success = false;
        }
    }

    // GetFormatTag
    {
        WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 2, 44100, 88200, 4, 16, 0 };
        if (GetFormatTag(&wfx) != WAVE_FORMAT_PCM)
        {
            printf("ERROR: Failed GetFormatTag PCM test\n");
            success = false;
        }

        wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        if (GetFormatTag(&wfx) != WAVE_FORMAT_IEEE_FLOAT)
        {
            printf("ERROR: Failed GetFormatTag IEEE float test\n");
            success = false;
        }

        WAVEFORMATEXTENSIBLE wfxext = { { WAVE_FORMAT_EXTENSIBLE, 2, 44100, 88200, 4, 16, 22 }, {}, 0, { 0x00000000, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } } };
        wfxext.SubFormat.Data1 = WAVE_FORMAT_PCM;
        if (GetFormatTag(reinterpret_cast<WAVEFORMATEX*>(&wfxext)) != WAVE_FORMAT_PCM)
        {
            printf("ERROR: Failed GetFormatTag extensible PCM test\n");
            success = false;
        }

        wfxext.SubFormat.Data1 = WAVE_FORMAT_IEEE_FLOAT;
        if (GetFormatTag(reinterpret_cast<WAVEFORMATEX*>(&wfxext)) != WAVE_FORMAT_IEEE_FLOAT)
        {
            printf("ERROR: Failed GetFormatTag extensible IEEE float test\n");
            success = false;
        }

        wfxext.SubFormat = {};
        if (GetFormatTag(reinterpret_cast<WAVEFORMATEX*>(&wfxext)) != 0)
        {
            printf("ERROR: Expected GetFormatTag 0 for mismatch GUID\n");
            success = false;
        }

        wfxext.Format.cbSize = 1;
        if (GetFormatTag(reinterpret_cast<WAVEFORMATEX*>(&wfxext)) != 0)
        {
            printf("ERROR: Expected GetFormatTag 0 for wrong size\n");
            success = false;
        }
    }

    #ifndef DIRECTX_TOOLKIT_IMPORT

    // CreateIntegerPCM
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        CreateIntegerPCM(wfx, 44100, 2, 16);

        if (!IsValid(wfx)
            || wfx->wFormatTag != WAVE_FORMAT_PCM || wfx->nChannels != 2
            || wfx->nSamplesPerSec != 44100 || wfx->wBitsPerSample != 16
            || wfx->nBlockAlign != 4 || wfx->nAvgBytesPerSec != 176400
            || wfx->cbSize != 0)
        {
            printf("ERROR: Failed CreateIntegerPCM test\n");
            success = false;
        }
    }

    // CreateFloatPCM
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        CreateFloatPCM(wfx, 44100, 2);

        if (!IsValid(wfx)
            || wfx->wFormatTag != WAVE_FORMAT_IEEE_FLOAT || wfx->nChannels != 2
            || wfx->nSamplesPerSec != 44100 || wfx->wBitsPerSample != 32
            || wfx->nBlockAlign != 8 || wfx->nAvgBytesPerSec != 352800
            || wfx->cbSize != 0)
        {
            printf("ERROR: Failed CreateFloatPCM test\n");
            success = false;
        }
    }

    // CreateADPCM
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        CreateADPCM(wfx, sizeof(buff), 44100, 2, 8);

        if (!IsValid(wfx)
            || wfx->wFormatTag != WAVE_FORMAT_ADPCM || wfx->nChannels != 2
            || wfx->nSamplesPerSec != 44100 || wfx->wBitsPerSample != 4
            || wfx->nBlockAlign != 20 || wfx->nAvgBytesPerSec != 110250
            || wfx->cbSize != 32)
        {
            printf("ERROR: Failed CreateADPCM test\n");
            success = false;
        }
    }

    #ifdef USING_XAUDIO2_9
    // CreateXWMA
    {
        char buff[64] = {};
        auto wfx = reinterpret_cast<WAVEFORMATEX*>(buff);
        CreateXWMA(wfx, 44100, 2, 8, 176400, false);

        if (!IsValid(wfx)
            || wfx->wFormatTag != WAVE_FORMAT_WMAUDIO2 || wfx->nChannels != 2
            || wfx->nSamplesPerSec != 44100 || wfx->wBitsPerSample != 16
            || wfx->nBlockAlign != 8 || wfx->nAvgBytesPerSec != 176400
            || wfx->cbSize != 0)
        {
            printf("ERROR: Failed CreateXWMA (2) test\n");
            success = false;
        }

        CreateXWMA(wfx, 44100, 2, 8, 176400, true);

        if (!IsValid(wfx)
            || wfx->wFormatTag != WAVE_FORMAT_WMAUDIO3 || wfx->nChannels != 2
            || wfx->nSamplesPerSec != 44100 || wfx->wBitsPerSample != 16
            || wfx->nBlockAlign != 8 || wfx->nAvgBytesPerSec != 176400
            || wfx->cbSize != 0)
        {
            printf("ERROR: Failed CreateXWMA (3) test\n");
            success = false;
        }
    }
    #endif

    #endif // !DIRECTX_TOOLKIT_IMPORT

    // invalid args
    #pragma warning(push)
    #pragma warning(disable:6385 6387)
    {
        WAVEFORMATEX* wfxNull = nullptr;
        if (IsValid(wfxNull))
        {
            printf("ERROR: Failed null wave format test\n");
            success = false;
        }

    #ifndef DIRECTX_TOOLKIT_IMPORT
        CreateIntegerPCM(wfxNull, 44100, 2, 16);
        CreateFloatPCM(wfxNull, 44100, 2);
        CreateADPCM(wfxNull, 64, 44100, 2, 8);
        #ifdef USING_XAUDIO2_9
        CreateXWMA(wfxNull, 44100, 2, 8, 176400, false);
        #endif

        try
        {
            WAVEFORMATEX invalid = {};
            CreateADPCM(&invalid, sizeof(invalid), 0, 0, 0);

            printf("ERROR: Failed to throw for small buffer for CreateADPCM\n");
        }
        catch(std::exception&)
        {
        }

        try
        {
            char buff[64] = {};
            auto invalid = reinterpret_cast<WAVEFORMATEX*>(buff);
            CreateADPCM(invalid, sizeof(buff), 0, 0, 0);

            printf("ERROR: Failed to throw for invalid samples per block CreateADPCM\n");
        }
        catch(std::exception&)
        {
        }
    #endif // !DIRECTX_TOOLKIT_IMPORT
    }
    #pragma warning(pop)

    return success;
}
