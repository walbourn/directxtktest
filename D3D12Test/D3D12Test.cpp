//--------------------------------------------------------------------------------------
// File: DX12Test.cpp
//
// Developer unit test for basic Direct3D 12 support
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#include <d3d12.h>
#include <dxgi1_4.h>
#include "DirectXColors.h"

#include <wrl/client.h>
#include <wrl/event.h>

#include "SimpleMath.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

// Helper sets a D3D resource name string (used by PIX and debug layer leak reporting).
inline void SetDebugObjectName(_In_ ID3D12Object* object, _In_z_ const wchar_t * name)
{
#if defined(_DEBUG) || defined(PROFILE)
    object->SetName( name );
#else
    UNREFERENCED_PARAMETER(object);
    UNREFERENCED_PARAMETER(name);
#endif
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}


int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow )
{
    HRESULT hr;

    wchar_t *const className = L"TestWindowClass";

    WNDCLASSEX wndClass = { 0 };

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = className;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wndClass);

    HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, L"Test Window", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                               CW_USEDEFAULT, CW_USEDEFAULT, 640, 640, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    RECT client;
    GetClientRect(hwnd, &client);

#ifdef _DEBUG
    // Enable the Direct3D 12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        hr = D3D12GetDebugInterface( IID_PPV_ARGS(&debugController) );
        if (SUCCEEDED(hr))
            debugController->EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr))
        return 1;

    ComPtr<ID3D12Device> device;
    hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
    if (FAILED(hr))
    {
        // Try WARP12 instead
        ComPtr<IDXGIAdapter> warpAdapter;
        hr = dxgiFactory->EnumWarpAdapter( IID_PPV_ARGS(&warpAdapter) );
        if (FAILED(hr))
            return 1;

        hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
        if (FAILED(hr))
        {
            MessageBox(hwnd, L"No Direct3D 12 device found; WARP12 not available", L"Test", MB_OK | MB_ICONERROR);
            return 1;
        }
    }

    ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue))))
        return 1;

    SetDebugObjectName(queue.Get(), L"Global");

    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = client.right;
    swapChainDesc.BufferDesc.Height = client.bottom;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> swapChain;
    if (FAILED(hr = dxgiFactory->CreateSwapChain(queue.Get(), &swapChainDesc, &swapChain)))
        return 1;

    ComPtr<ID3D12CommandAllocator> allocator;
    if (FAILED(hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))))
        return 1;

    SetDebugObjectName(allocator.Get(), L"Global");

    ComPtr<ID3D12DescriptorHeap> descHeap;
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = { };
    descHeapDesc.NumDescriptors = 1;
    descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(hr = device->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&descHeap))))
        return 1;

    SetDebugObjectName(descHeap.Get(), L"Global");

    ComPtr<ID3D12GraphicsCommandList> cmdList;
    if (FAILED(hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList))))
        return 1;

    SetDebugObjectName(cmdList.Get(), L"Global");

    ComPtr<ID3D12Resource> backBufferTexture;
    if (FAILED(hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferTexture))))
        return 1;

    device->CreateRenderTargetView(backBufferTexture.Get(), nullptr, descHeap->GetCPUDescriptorHandleForHeapStart());

    ComPtr<ID3D12Fence> fence;
    if (FAILED(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
        return 1;

    UINT64 currentFence = 1;

    if (FAILED(hr = cmdList->Close()))
        return 1;

    Microsoft::WRL::Wrappers::Event frameEnd(CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS));
    if (!frameEnd.IsValid())
        return 1;

    bool quit = false;

    size_t frame = 0;

    int swapBufferIndex = 0;

    while (!quit)
    {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                quit = true;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        static float t = 0.0f;
        static uint64_t timeStart = 0;
        uint64_t timeCur = GetTickCount64();
        if( timeStart == 0 )
            timeStart = timeCur;
        t = ( timeCur - timeStart ) / 1000.0f;

        hr = allocator->Reset();
        if (FAILED(hr))
            break;

        hr = cmdList->Reset(allocator.Get(), nullptr);
        if (FAILED(hr))
            break;

        D3D12_RESOURCE_BARRIER barrier;
        memset(&barrier, 0, sizeof(barrier));
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = backBufferTexture.Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        cmdList->ResourceBarrier(1, &barrier);

        cmdList->ClearRenderTargetView(descHeap->GetCPUDescriptorHandleForHeapStart(), Colors::CornflowerBlue, 0, nullptr);

        // Set the viewport and scissor rect.

        Viewport viewPort(0.0f, 0.0f, static_cast<float>(client.right), static_cast<float>(client.bottom));
        cmdList->RSSetViewports(1, viewPort.Get12());
        cmdList->RSSetScissorRects(1, &client);

        // Scene is blank for this test

        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        cmdList->ResourceBarrier(1, &barrier);

        hr = cmdList->Close();
        if (FAILED(hr))
            break;

        ID3D12CommandList* clist = cmdList.Get();
        queue->ExecuteCommandLists(1, &clist);

        hr = swapChain->Present(1, 0);
        if (FAILED(hr))
            break;

        ++frame;

        swapBufferIndex = (1 + swapBufferIndex) % 2;
        hr = swapChain->GetBuffer(swapBufferIndex, IID_PPV_ARGS(&backBufferTexture));
        if (FAILED(hr))
            break;

        device->CreateRenderTargetView(backBufferTexture.Get(), nullptr, descHeap->GetCPUDescriptorHandleForHeapStart());

        const UINT64 fenceValue = currentFence;
        hr = queue->Signal(fence.Get(), fenceValue);
        ++currentFence;

        if (fence->GetCompletedValue() < fenceValue)
        {
            hr = fence->SetEventOnCompletion(fenceValue, frameEnd.Get());
            WaitForSingleObject(frameEnd.Get(), INFINITE);
        }
    }

    return 0;
}
