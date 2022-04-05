//-------------------------------------------------------------------------------------
// SimpleMathTestCPP17.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

// This file has /std:c++17 when building with VS 2017 or later

#if __cplusplus != 201703L
#if defined(_MSVC_LANG) && _MSVC_LANG == 201703L
#error Add /Zc:__cplusplus to the build settings
#endif
#error This file should be built with C++17 mode enabled
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
