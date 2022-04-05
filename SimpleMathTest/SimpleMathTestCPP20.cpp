//-------------------------------------------------------------------------------------
// SimpleMathTestCPP20.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

// This file has /std:c++20 when building with VS 2022 or later

#if __cplusplus != 202002L
#if defined(_MSVC_LANG) && _MSVC_LANG == 202002L
#error Add /Zc:__cplusplus to the build settings
#endif
#error This file should be built with C++20 mode enabled
#endif

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include "SimpleMath.h"
