#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <stdio.h>
#include <tuple>


#define SAFE_RELEASE(x) if(x) (x)->Release(); x=nullptr
inline void ThrowFailedHR(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw hr;
	}
}

typedef unsigned int uint;
const uint WINDOW_WIDTH = 1280;
const uint WINDOW_HEIGHT = 720;
const uint SWAP_BUFFER_COUNT = 2;
const uint SIGNAL_ZERO = 0;
const uint SIGNAL_ONE = 1;

struct Global {
	IDXGIFactory2* factory;
	IDXGIAdapter* adapter;
	ID3D12Device* device;
	ID3D12DebugDevice* debugDevice;

	ID3D12CommandQueue* commandQueue;
	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList* commandList;
	
	IDXGISwapChain4* swapChain;
	uint currentBufferIndex;
	ID3D12Resource* renderTargets[SWAP_BUFFER_COUNT];
	ID3D12DescriptorHeap* rtvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[SWAP_BUFFER_COUNT];

	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pipeline;

	ID3D12Resource* vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ID3D12Fence* fence;
	HANDLE fenceEvent;

	~Global() {
		SAFE_RELEASE(factory);
		SAFE_RELEASE(adapter);
		SAFE_RELEASE(device);
		SAFE_RELEASE(commandQueue);
		SAFE_RELEASE(commandAllocator);
		SAFE_RELEASE(commandList);
		SAFE_RELEASE(swapChain);
		for (uint i = 0; i < SWAP_BUFFER_COUNT; ++i) {
			SAFE_RELEASE(renderTargets[i]);
		}
		SAFE_RELEASE(rtvHeap);
		SAFE_RELEASE(rootSignature);
		SAFE_RELEASE(pipeline);
		SAFE_RELEASE(vertexBuffer);
		SAFE_RELEASE(fence);
		g.debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		SAFE_RELEASE(debugDevice);
	}
} g;


HWND createWindow(const char* winTitle)
{
	WNDCLASSA wc = {
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
			return DefWindowProc(hWnd, message, wParam, lParam);
		},
		.hInstance = GetModuleHandle(nullptr),
		.hCursor = LoadCursor(nullptr, IDC_CROSS),
		.lpszClassName = "anything"
	};
	RegisterClass(&wc);

	RECT r{ .right = (LONG)WINDOW_WIDTH, .bottom = (LONG)WINDOW_HEIGHT };
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, false);

	HWND hwnd = CreateWindowA(
		wc.lpszClassName,
		winTitle,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		r.right - r.left,
		r.bottom - r.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr);

	return hwnd;
}

void createDevice(bool needRaytracing)
{
#ifdef _DEBUG
	ID3D12Debug* debuger;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debuger))))
	{
		debuger->EnableDebugLayer();
		SAFE_RELEASE(debuger);
	}
#endif
	ThrowFailedHR(CreateDXGIFactory1(IID_PPV_ARGS(&g.factory)));

	IDXGIAdapter* tempAdapter = nullptr;
	ID3D12Device* tempDevice = nullptr;
	for (uint i = 0; g.factory->EnumAdapters(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		if (FAILED(D3D12CreateDevice(tempAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&tempDevice))))
		{
			SAFE_RELEASE(tempAdapter);
			SAFE_RELEASE(tempDevice);
			continue;
		}

		if (needRaytracing)
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5;
			HRESULT hr = tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
			if (SUCCEEDED(hr) && features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
			{
				break;
			}
		}
		else
		{
			break;
		}

		SAFE_RELEASE(tempAdapter);
		SAFE_RELEASE(tempDevice);
	}

	if (tempDevice == nullptr) {
		printf("For this application, you need a graphic card supporting hardware-accelerated raytracing API.\n");
		throw;
	}

	g.adapter = tempAdapter;
	g.device = tempDevice;
#ifdef _DEBUG
	g.device->QueryInterface(IID_PPV_ARGS(&g.debugDevice));
#endif
}

void createCommandCenter()
{
	D3D12_COMMAND_QUEUE_DESC desc = { .Type = D3D12_COMMAND_LIST_TYPE_DIRECT };
	ThrowFailedHR(g.device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g.commandQueue)));
	ThrowFailedHR(g.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g.commandAllocator)));
	ThrowFailedHR(g.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g.commandAllocator, nullptr, IID_PPV_ARGS(&g.commandList)));
	ThrowFailedHR(g.commandList->Close());
}

void createSwapChain(HWND hwnd, bool allowTearing = false)
{
	DXGI_SWAP_CHAIN_DESC1 scDesc = {
		.Width = WINDOW_WIDTH,
		.Height = WINDOW_HEIGHT,
		.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		.SampleDesc = { .Count = 1, },
		.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount = SWAP_BUFFER_COUNT,
		.Scaling = DXGI_SCALING_NONE,
		.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		.Flags = allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u,
	};

	ThrowFailedHR(g.factory->CreateSwapChainForHwnd(
		g.commandQueue,
		hwnd,
		&scDesc,
		nullptr, nullptr,
		(IDXGISwapChain1**)(IDXGISwapChain3**)&g.swapChain));

	g.currentBufferIndex = g.swapChain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC hDesc = {
		.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		.NumDescriptors = SWAP_BUFFER_COUNT,
	};
	ThrowFailedHR(g.device->CreateDescriptorHeap(&hDesc, IID_PPV_ARGS(&g.rtvHeap)));

	D3D12_CPU_DESCRIPTOR_HANDLE heapAddress = g.rtvHeap->GetCPUDescriptorHandleForHeapStart();
	uint rtvSize = g.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (uint i = 0; i < SWAP_BUFFER_COUNT; ++i)
	{
		ThrowFailedHR(g.swapChain->GetBuffer(i, IID_PPV_ARGS(&g.renderTargets[i])));
		g.rtvs[i] = heapAddress;
		g.device->CreateRenderTargetView(g.renderTargets[i], nullptr, heapAddress);
		heapAddress.ptr += rtvSize;
	}
}

std::tuple<ID3DBlob*, ID3DBlob*> getShader()
{
	const char code[] = R"(
		cbuffer cb0 : register(b10, space100)
		{
			float t;
		};

		struct PSInput {
			float4 position : SV_POSITION;
			float4 color : COLOR;
		};

		PSInput VSMain(float4 position : POSITION, float4 color : COLOR) {
			PSInput result;
			result.position = position + float4(t, 0, 0, 0);
			result.color = color;
			return result;
		}

		float4 PSMain(PSInput input) : SV_TARGET {
			return input.color;
		}
	)";
	const size_t codeLength = sizeof(code);
	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* vs;
	ID3DBlob* ps;
	ID3DBlob* error = nullptr;
	HRESULT hr = D3DCompile(code, codeLength, NULL, NULL, NULL, "VSMain", "vs_5_1", compileFlags, 0, &vs, &error);
	if (FAILED(hr) || error)
	{
		if(error) printf((char*)error->GetBufferPointer());
		throw hr;
	}
	hr = D3DCompile(code, codeLength, NULL, NULL, NULL, "PSMain", "ps_5_1", compileFlags, 0, &ps, &error);
	if (FAILED(hr) || error)
	{
		if (error) printf((char*)error->GetBufferPointer());
		throw hr;
	}

	return { vs, ps };
}

void createRootSignature()
{
	D3D12_ROOT_PARAMETER param = {
		.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
		.Constants = {// -> bind to register(b10, space100) in .hlsl
			.ShaderRegister = 10,  
			.RegisterSpace = 100,
			.Num32BitValues = 1,   
		},
		.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
	};
	D3D12_ROOT_SIGNATURE_DESC desc = {
		.NumParameters = 1,
		.pParameters = &param,
		.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
	};
	ID3DBlob* blob, * error = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error);
	if (FAILED(hr) || error)
	{
		if (error) printf((char*)error->GetBufferPointer());
		throw hr;
	}

	ThrowFailedHR(g.device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&g.rootSignature)));
	SAFE_RELEASE(blob);
}

void createPipeline(ID3DBlob* vs, ID3DBlob* ps)
{
	D3D12_INPUT_ELEMENT_DESC vertexLayout[] = {
		{
			.SemanticName = "POSITION",
			.Format = DXGI_FORMAT_R32G32B32_FLOAT,
		},
		{
			.SemanticName = "COLOR",
			.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
			.AlignedByteOffset = sizeof(float) * 3,
		},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
		.pRootSignature = g.rootSignature,
		.VS = {
			.pShaderBytecode = vs->GetBufferPointer(),
			.BytecodeLength = vs->GetBufferSize(),
		},
		.PS = {
			.pShaderBytecode = ps->GetBufferPointer(),
			.BytecodeLength = ps->GetBufferSize(),
		},
		.BlendState = {
			.RenderTarget = { { .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL } },
		},
		.SampleMask = uint(-1),
		.RasterizerState = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_NONE,
			.DepthClipEnable = TRUE,
		},
		.InputLayout = {
			.pInputElementDescs = vertexLayout,
			.NumElements = 2,
		},
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM },
		.SampleDesc = { .Count = 1, },
	};

	ThrowFailedHR(g.device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&g.pipeline)));
	SAFE_RELEASE(vs);
	SAFE_RELEASE(ps);
}

void createBuffer()
{
	float aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

	const float vertices[] = {
		// pos  						color
		 0.00f,  0.25f * aspect, 0.0f,	1,0,0,0,
		 0.25f, -0.25f * aspect, 0.0f, 	0,1,0,0,
		-0.25f, -0.25f * aspect, 0.0f, 	0,0,1,0,
	};

	const D3D12_HEAP_PROPERTIES heapProps = {
		.Type = D3D12_HEAP_TYPE_UPLOAD,
	};

	const D3D12_RESOURCE_DESC bufferDesc = {
		.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		.Width = sizeof(vertices),
		.Height = 1,
		.DepthOrArraySize = 1,
		.MipLevels = 1,
		.SampleDesc = {.Count = 1,},
		.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
	};

	g.device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&g.vertexBuffer));

	void* dstAddress;
	D3D12_RANGE range = {};
	g.vertexBuffer->Map(0, &range, &dstAddress);
	memcpy(dstAddress, vertices, sizeof(vertices));
	g.vertexBuffer->Unmap(0, nullptr);

	g.vertexBufferView = {
		.BufferLocation = g.vertexBuffer->GetGPUVirtualAddress(),
		.SizeInBytes = sizeof(vertices),
		.StrideInBytes = sizeof(float) * (3 + 4),
	};
}

void createFence()
{
	ThrowFailedHR(g.device->CreateFence(SIGNAL_ZERO, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS((&g.fence))));
	BOOL manualReset = FALSE;
	BOOL initialState = FALSE;
	g.fenceEvent = CreateEvent(nullptr, manualReset, initialState, nullptr);
}

void waitCommandQueue()
{
	ThrowFailedHR(g.commandQueue->Signal(g.fence, SIGNAL_ONE));

	if (g.fence->GetCompletedValue() != SIGNAL_ONE)
	{
		ThrowFailedHR(g.fence->SetEventOnCompletion(SIGNAL_ONE, g.fenceEvent));
		WaitForSingleObject(g.fenceEvent, INFINITE);
	}

	ThrowFailedHR(g.commandQueue->Signal(g.fence, SIGNAL_ZERO));
}

void render()
{
	ThrowFailedHR(g.commandAllocator->Reset());
	ThrowFailedHR(g.commandList->Reset(g.commandAllocator, nullptr));
	{
		const D3D12_VIEWPORT viewport = {
			.Width = (float)WINDOW_WIDTH,
			.Height = (float)WINDOW_HEIGHT,
		};

		const D3D12_RECT scissorRect = {
			.right = WINDOW_WIDTH,
			.bottom = WINDOW_HEIGHT,
		};

		g.commandList->SetPipelineState(g.pipeline);
		g.commandList->SetGraphicsRootSignature(g.rootSignature);
		g.commandList->RSSetViewports(1, &viewport);
		g.commandList->RSSetScissorRects(1, &scissorRect);
		g.commandList->OMSetRenderTargets(1, &g.rtvs[g.currentBufferIndex], false, nullptr);
		g.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g.commandList->IASetVertexBuffers(0, 1, &g.vertexBufferView);

		static float t = 0.0;
		g.commandList->SetGraphicsRoot32BitConstant(0, *reinterpret_cast<uint*>(&t), 0);
		t += 0.001f;
		
		D3D12_RESOURCE_BARRIER barrier = {
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Transition = {
				.pResource = g.renderTargets[g.currentBufferIndex],
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
				.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
			},
		};
		g.commandList->ResourceBarrier(1, &barrier);
		{
			const float clearcolor[] = { 0.05f, 0.5f, 0.05f, 1.0f };
			g.commandList->ClearRenderTargetView(g.rtvs[g.currentBufferIndex], clearcolor, 0, NULL);
			g.commandList->DrawInstanced(3, 1, 0, 0);
		}
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		g.commandList->ResourceBarrier(1, &barrier);
	}
	ThrowFailedHR(g.commandList->Close());

	ID3D12CommandList* pCommandLists[] = { g.commandList };
	g.commandQueue->ExecuteCommandLists(1, pCommandLists);

	ThrowFailedHR(g.swapChain->Present(1, 0));
	g.currentBufferIndex = g.swapChain->GetCurrentBackBufferIndex();
}

int main()
{
	HWND hwnd = createWindow("Hello Triangle!");
	createDevice(false);
	createCommandCenter();
	createSwapChain(hwnd);
	auto [vs, ps] = getShader();
	createRootSignature();
	createPipeline(vs, ps);
	createBuffer();
	createFence();
	waitCommandQueue();

	while (IsWindow(hwnd))
	{
		render();
		waitCommandQueue();

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}