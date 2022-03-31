//--------------------------------------------------------------------------------------
// File: DirectXTKTest.h
//
// Helper header for DirectX Tool Kit for DX11 test suite
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//-------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#define XBOX
#define COREWINDOW
#include "DeviceResourcesXDK.h"

#elif defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#define UWP
#define COREWINDOW
#define LOSTDEVICE
#include "DeviceResourcesUWP.h"

#else
#define PC
#define LOSTDEVICE
#include "DeviceResourcesPC.h"

#endif
