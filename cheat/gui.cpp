#include "gui.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include <fstream>
#include "globals.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

namespace shit
{
	int tab;
}

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter)
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;

	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click points
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}

	}return 0;

	}

	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = "class001";
	windowClass.hIconSm = 0;

	RegisterClassEx(&windowClass);

	window = CreateWindowEx(
		0,
		"class001",
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);

	ImGuiStyle& Style = ImGui::GetStyle();

	ImGui::GetStyle().WindowPadding = ImVec2(15, 15);
	ImGui::GetStyle().WindowRounding = 10.0f;
	ImGui::GetStyle().FramePadding = ImVec2(5, 5);
	ImGui::GetStyle().FrameRounding = 10.0f; // Make all elements (checkboxes, etc) circles
	ImGui::GetStyle().ItemSpacing = ImVec2(12, 8);
	ImGui::GetStyle().ItemInnerSpacing = ImVec2(8, 6);
	ImGui::GetStyle().IndentSpacing = 25.0f;
	ImGui::GetStyle().ScrollbarSize = 15.0f;
	ImGui::GetStyle().ScrollbarRounding = 9.0f;
	ImGui::GetStyle().GrabMinSize = 20.0f; // Make grab a circle
	ImGui::GetStyle().GrabRounding = 12.0f;
	ImGui::GetStyle().PopupRounding = 7.f;
	ImGui::GetStyle().Alpha = 1.0;

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(1.00f, 0.00f, 0.00f, 0.30f);
	colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.00f, 0.50f, 0.95f, 0.30f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.50f, 0.95f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(1.000f, 0.777f, 0.578f, 0.780f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.987f, 0.611f, 0.600f);
	colors[ImGuiCol_Button] = ImVec4(0.90f, 0.45f, 0.65f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.65f, 0.35f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.84f, 0.97f, 0.01f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(1.00f, 1.00f, 1.00f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.20f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(1.00f, 0.54f, 0.01f, 0.71f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.96f, 0.73f, 0.09f, 0.90f);
	colors[ImGuiCol_TabActive] = ImVec4(1.00f, 0.97f, 0.00f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.92f, 0.93f, 0.94f, 0.99f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.80f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			isRunning = !isRunning;
			return;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();



}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

void gui::Render() noexcept
{

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"BubbleNight",
		&isRunning,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove 
	);

	{ImGui::SameLine();
	if (ImGui::Button("AimBot", ImVec2(105, 30)))
	{
		shit::tab = 0;
	}
	ImGui::SameLine();
	if (ImGui::Button("Visuals", ImVec2(105, 30)))
	{
		shit::tab = 1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Misc", ImVec2(105, 30)))
	{
		shit::tab = 2;
	}
	ImGui::SameLine();
	if (ImGui::Button("Credits", ImVec2(105, 30)))
	{
		shit::tab = 3;
	}
	}
	ImGui::Separator;

	if (shit::tab == 0)
	{
		ImGui::Text("---AimBot---");
		ImGui::Checkbox("aimbot", &globals::aimbot);
		ImGui::SliderFloat("Smoothening", globals::Smoothening, 0.57f, 50.0f);
		ImGui::SliderFloat("FOV", globals::FOV, 1.0f, 200.0f);

		struct Funcs { static bool ItemGetter(void* data, int n, const char** out_str) { *out_str = ((const char**)data)[n]; return true; } };
		const char* items[] = { "HEAD", "NECK", "CHEST", "LEGS" };
		static int item_current_idx = 0;
		const char* combo_preview_value = items[item_current_idx];
		static int item_current = 0;

		ImGui::Combo("Aim Part", &item_current, &Funcs::ItemGetter, items, IM_ARRAYSIZE(items));
		{
			switch (item_current)
			{
			case 0:
				globals::AimPart = 8; //head
				break;
			case 1:
				globals::AimPart = 7; //neck
				break;
			case 2:
				globals::AimPart = 5; //chest
				break;
			case 3:
				globals::AimPart = 77; //legs
				break;
			default:
				break;
			}

		}

		struct Funcs1 { static bool ItemGetter1(void* data, int n, const char** out_str) { *out_str = ((const char**)data)[n]; return true; } };

		const char* items1[] = { "Right Mouse", "Left Mouse" };
		static int item_current_idx1 = 0;
		const char* combo_preview_value1 = items1[item_current_idx1];
		static int item_current1 = 0;

		ImGui::Combo("Aim Key", &item_current1, &Funcs1::ItemGetter1, items1, IM_ARRAYSIZE(items1));
		{
			switch (item_current1)
			{
			case 0:
				globals::AimKey = VK_RBUTTON;
				break;
			case 1:
				globals::AimKey = VK_LBUTTON;
				break;
			default:
				break;
			}

		}
		ImGui::Checkbox("TriggerBot", &globals::Triggerbot);

		struct Funcs2 { static bool ItemGetter2(void* data, int n, const char** out_str) { *out_str = ((const char**)data)[n]; return true; } };

		const char* items2[] = { "Shift", "Right Mouse", "ALT", "Mouse 5"};
		static int item_current_idx2 = 0;
		const char* combo_preview_value2 = items2[item_current_idx2];
		static int item_current2 = 0;

		ImGui::Combo("TriggerBot Key", &item_current2, &Funcs2::ItemGetter2, items2, IM_ARRAYSIZE(items2));
		{
			switch (item_current2)
			{
			case 0:
				globals::TriggerKey = VK_SHIFT;
				break;
			case 1:
				globals::TriggerKey = VK_RBUTTON;
				break;
			case 2:
				globals::TriggerKey = VK_MENU;
				break;
			case 3:
				globals::TriggerKey = VK_XBUTTON2;
				break;
			default:
				break;
			}

		}
	}

	if (shit::tab == 1)
	{
		ImGui::Text("---Visuals---");
		ImGui::Checkbox("glow", &globals::glow);
		ImGui::ColorEdit4("glow color", globals::glowColor);
		ImGui::Checkbox("HP Chams", &globals::HPChams);
		ImGui::Checkbox("Chams", &globals::Chams);
		ImGui::ColorEdit3("Chams color", globals::ChamsColor);
		ImGui::SliderFloat("Brightness", globals::Brightness, 0.0f, 25.0f);
		
	}

	if (shit::tab == 2)
	{
		ImGui::Text("---Misc---");
		ImGui::Checkbox("Bhop", &globals::Bhop);
		ImGui::Checkbox("Skin Changer (Beta)", &globals::Skin);
		ImGui::Checkbox("Anti-Flash", &globals::Flash);
		ImGui::Checkbox("Less-Flash", &globals::LessFlash);
		ImGui::SliderFloat("Flash %", globals::FlashPer, 0.0f, 255.0f);
		ImGui::Checkbox("Radar", &globals::Radar);
		ImGui::Checkbox("FOV (Broken)", &globals::gameFOV);
		ImGui::SliderInt("FOV (Broken)", &globals::gameFOVslider, 10, 170);
		ImGui::Checkbox("Spectators (beta)", &globals::spec);
	}

	if (shit::tab == 3)
	{
		ImGui::Text("---Credits---");
		ImGui::Text("Made by");
		ImGui::Text("Ice_cream_sandwitch#4865");
		ImGui::Text("-----------");
		ImGui::Text("Future updates");
		ImGui::Text("1. Better ESP");
		ImGui::Text("2. Better Spectator List");
		ImGui::Text("3. Better Skin Changer");
		ImGui::Text("dm me for your things you want added");
	}

	if (globals::hasSpectators == true)
		ImGui::Text("You Have Spectators");
	else
		ImGui::Text("No Spectators");

	ImGui::End();
}
