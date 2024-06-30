#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <windows.h>
#include <memory>
#include <iostream>

#include "Example.h"

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 콘솔창이 있으면 디버깅에 유리합니다.
int main()
{
	const int width = 1280, height = 960;
	const int canvasWidth = width / 80, canvasHeight = height / 80;

	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC,
		WndProc,
		0L,
		0L,
		GetModuleHandle(NULL),
		NULL,
		NULL,
		NULL,
		NULL,
		L"HongLabGraphics", // lpszClassName, L-string
		NULL
	};

	RegisterClassEx(&wc);

	// 실제로 그려지는 해상도를 설정하기 위해
	RECT wr = { 0, 0, width, height };    // set the size, but not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

	// 설정에 따라 조정된 RECT(메뉴바 추가여부)를 다시 받아서 윈도우 생성시 넣어줌
	HWND hwnd = CreateWindow(
		wc.lpszClassName,
		L"HongLabGraphics Example",
		WS_OVERLAPPEDWINDOW,
		100, // 윈도우 좌측 상단의 x 좌표
		100, // 윈도우 좌측 상단의 y 좌표
		wr.right - wr.left, // 윈도우 가로 방향 해상도
		wr.bottom - wr.top, // 윈도우 세로 방향 해상도
		NULL,
		NULL,
		wc.hInstance,
		NULL);

	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	auto example = std::make_unique<Example>(hwnd, width, height, canvasWidth, canvasHeight);
	//Example* example = new Example(....);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.DisplaySize = ImVec2(width, height);
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplDX11_Init(example->device, example->deviceContext);
	ImGui_ImplWin32_Init(hwnd);

	float canvasColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };//TODO: 실습문제용 임시코드

	// Main message loop
	MSG msg = {};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Start the Dear ImGui frame
			// gui 프레임 준비
			ImGui_ImplDX11_NewFrame();//TODO: IMGUI 사용
			ImGui_ImplWin32_NewFrame();
			// 프레임만들어서 시작하여 함수들 사용후
			ImGui::NewFrame();
			ImGui::Begin("Background Color");
			ImGui::SliderFloat3("RGB(0.0->1.0)", example->backgroundColor, 0.0f, 1.0f);
			// 종료하고 메모리에 렌더
			ImGui::End();
			ImGui::Render();
			// 프레임 업데이트 후 메모리에 렌더하여 swap chain 하고나서 draw
			example->Update();
			example->Render();

			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());//TODO: IMGUI 사용

			// switch the back buffer and the front buffer
			example->swapChain->Present(1, 0);


			// ImGui는 렌더링 결과를 후처리하기 위해
			// 후면 버퍼에 그려진 ImGui 요소를 직접적으로 화면에 렌더링하는 역할을 하기에
			// 먼저 렌더를 실행하여야 한다
		}

	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	example->Clean();
	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Windows procedure
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		// Reset and resize swapchain
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu 
			return 0;
		break;
	case WM_MOUSEMOVE:
		//std::cout << "Mouse " << LOWORD(lParam) << " " << HIWORD(lParam) << std::endl;
		break;
	case WM_LBUTTONUP:
		//std::cout << "WM_LBUTTONUP Left mouse button" << std::endl;
		break;
	case WM_RBUTTONUP:
		//std::cout << "WM_RBUTTONUP Right mouse button" << std::endl;
		break;
	case WM_KEYDOWN:
		//std::cout << "WM_KEYDOWN " << (int)wParam << std::endl;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}

	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
