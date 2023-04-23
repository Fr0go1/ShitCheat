#include "hacks.h"
#include "gui.h"
#include "globals.h"
#include "vector.h"
#include <thread>
#include <array>
#include <iostream>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

struct Color
{
	std::uint8_t r{ }, g{ }, b{ };
};

constexpr const int GetWeaponPaint(const short& itemDefinition)
{
	switch (itemDefinition)
	{
	case 1: return 527;
	case 4: return 572;
	case 7: return 259;
	case 9: return 344;
	case 16: return 588;
	case 61: return 637;
	default: return 0;
	}
}

void hacks::VisualsThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

		if (!localPlayer)
			continue;

		const auto glowManager = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwGlowObjectManager);

		if (!glowManager)
			continue;

		const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

		for (auto i = 1; i <= 32; ++i)
		{
			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

			if (!player)
				continue;

			const auto team = mem.Read<std::int32_t>(player + offsets::m_iTeamNum);

			if (team == localTeam)
				continue;

			const auto lifeState = mem.Read<std::int32_t>(player + offsets::m_lifeState);

			if (lifeState != 0)
				continue;

			if (globals::glow)
			{
				const auto glowIndex = mem.Read<std::int32_t>(player + offsets::m_iGlowIndex);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x8, globals::glowColor[0]); // red
				mem.Write(glowManager + (glowIndex * 0x38) + 0xC, globals::glowColor[1]); // green
				mem.Write(glowManager + (glowIndex * 0x38) + 0x10, globals::glowColor[2]); // blue
				mem.Write(glowManager + (glowIndex * 0x38) + 0x14, globals::glowColor[3]); // alpha

				mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x29, false);
			}

		}
	}
}

void hacks::AimbotThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::aimbot)
		{
			// aimbot key
			if (!GetAsyncKeyState(globals::AimKey))
				continue;

			// get local player
			const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
			const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

			// eye position = origin + viewOffset
			const auto &localEyePosition = mem.Read<Vector3>(localPlayer + offsets::m_vecOrigin) +
				mem.Read<Vector3>(localPlayer + offsets::m_vecViewOffset);

			const auto clientState = mem.Read<std::uintptr_t>(globals::engineAddress + offsets::dwClientState);

			const auto localPlayerId =
				mem.Read<std::int32_t>(clientState + offsets::dwClientState_GetLocalPlayer);

			const auto viewAngles = mem.Read<Vector3>(clientState + offsets::dwClientState_ViewAngles);
			const auto &aimPunch = mem.Read<Vector3>(localPlayer + offsets::m_aimPunchAngle) * 2;

			// aimbot fov
			auto bestFov = globals::FOV[0];
			auto bestAngle = Vector3{ };

			for (auto i = 1; i <= 32; ++i)
			{
				const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

				if (mem.Read<std::int32_t>(player + offsets::m_iTeamNum) == localTeam)
					continue;

				if (mem.Read<bool>(player + offsets::m_bDormant))
					continue;

				if (mem.Read<std::int32_t>(player + offsets::m_lifeState))
					continue;

				if (mem.Read<std::int32_t>(player + offsets::m_bSpottedByMask) & (1 << localPlayerId))
				{
					const auto boneMatrix = mem.Read<std::uintptr_t>(player + offsets::m_dwBoneMatrix);

					// pos of player head in 3d space
					// 8 is the head bone index :)
					const auto playerHeadPosition = Vector3{
						mem.Read<float>(boneMatrix + 0x30 * globals::AimPart + 0x0C),
						mem.Read<float>(boneMatrix + 0x30 * globals::AimPart + 0x1C),
						mem.Read<float>(boneMatrix + 0x30 * globals::AimPart + 0x2C)
					};
					const auto angle = CalculateAngle(
						localEyePosition,
						playerHeadPosition,
						viewAngles + aimPunch
					);

					const auto fov = std::hypot(angle.x, angle.y);
					if (fov < bestFov)
					{
						bestFov = fov;
						bestAngle = angle;
					}
				}

			}


			// if we have a best angle, do aimbot
			if (!bestAngle.IsZero())
				mem.Write<Vector3>(clientState + offsets::dwClientState_ViewAngles, viewAngles + bestAngle / globals::Smoothening[0]); // smoothing
		}
	}
}


void hacks::BhopThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::Bhop)
		{
			const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			if (!localPlayer)
				continue;

			const auto localPlayerFlags = mem.Read<std::uintptr_t>(localPlayer + offsets::m_fFlags);

			// bhop
			if (GetAsyncKeyState(VK_SPACE))
				(localPlayerFlags & (1 << 0)) ?
				mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceJump, 6):
				mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceJump, 4);
		}
	}
}



void hacks::ChamsThread(const Memory& mem) noexcept
{
	constexpr const auto TeamColor = Color{ 0, 0, 0 };
	constexpr const auto EnemyColor = Color{ 0, 255, 0 };

	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

		const auto& localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

		for (auto i = 1; i <= 32; ++i)
		{
			const auto& entity = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

			if (globals::Chams)
			{

				if (mem.Read<std::int32_t>(entity + offsets::m_iTeamNum) == localTeam)
				{
					//mem.Write<Color>(entity + offsets::m_clrRender, TeamColor);
				}
				else
				{
					Color chamsColor = Color{
						static_cast<unsigned char>(globals::ChamsColor[0] * 255),
						static_cast<unsigned char>(globals::ChamsColor[1] * 255),
						static_cast<unsigned char>(globals::ChamsColor[2] * 255)
					};

					mem.Write<Color>(entity + offsets::m_clrRender, chamsColor);
				}

				float brightness = globals::Brightness[0];
				const auto _this = static_cast<std::uintptr_t>(globals::engineAddress + offsets::model_ambient_min - 0x2c);
				mem.Write<std::int32_t>(globals::engineAddress + offsets::model_ambient_min, *reinterpret_cast<std::uintptr_t*>(&brightness) ^ _this);
			}

			else if (globals::HPChams)
			{

				int hp = mem.Read<int>((entity + offsets::m_iHealth));

				constexpr const auto GreenColor = Color{ 0, 255, 0 };
				constexpr const auto YellowColor = Color{ 255, 255, 0 };
				constexpr const auto RedColor = Color{ 255, 0, 0 };

				Color ChangeColor;

				if (hp > 66)
					ChangeColor = GreenColor;
				else if (hp <= 66 && hp >= 33)
					ChangeColor = YellowColor;
				else
					ChangeColor = RedColor;

				if (mem.Read<std::int32_t>(entity + offsets::m_iTeamNum) == localTeam)
				{
					//mem.Write<Color>(entity + offsets::m_clrRender, TeamColor);
				}
				else
				{
					mem.Write<int>(entity + offsets::m_clrRender, ChangeColor.r);
					mem.Write<int>(entity + offsets::m_clrRender + 1, ChangeColor.g);
					mem.Write<int>(entity + offsets::m_clrRender + 2, ChangeColor.b);
				}

				float brightness = globals::Brightness[0];
				const auto _this = static_cast<std::uintptr_t>(globals::engineAddress + offsets::model_ambient_min - 0x2c);
				mem.Write<std::int32_t>(globals::engineAddress + offsets::model_ambient_min, *reinterpret_cast<std::uintptr_t*>(&brightness) ^ _this);
			}

			else
			{
				float brightness = 0.0f;
				const auto _this = static_cast<std::uintptr_t>(globals::engineAddress + offsets::model_ambient_min - 0x2c);
				mem.Write<std::int32_t>(globals::engineAddress + offsets::model_ambient_min, *reinterpret_cast<std::uintptr_t*>(&brightness) ^ _this);

				const auto& entity = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

				mem.Write<Color>(entity + offsets::m_clrRender, Color{ 255, 255, 255 });
			}
		}
	}
}

void hacks::SkinThread(const Memory& mem) noexcept
{

	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2));

		const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		const auto& weapons = mem.Read<std::array<unsigned long, 8>>(localPlayer + offsets::m_hMyWeapons);

		for (const auto& handle : weapons)
		{

			if (globals::Skin)
			{
				const auto& weapon = mem.Read<std::uintptr_t>((globals::clientAddress + offsets::dwEntityList + (handle & 0xFFF) * 0x10) - 0x10);

				if (!weapon)
					continue;

				if (const auto paint = GetWeaponPaint(mem.Read<short>(weapon + offsets::m_iItemDefinitionIndex)))
				{
					const bool shouldUpdate = mem.Read<std::int32_t>(weapon + offsets::m_nFallbackPaintKit) != paint;

					mem.Write<std::int32_t>(weapon + offsets::m_iItemIDHigh, -1);

					mem.Write<std::int32_t>(weapon + offsets::m_nFallbackPaintKit, paint);
					mem.Write<float>(weapon + offsets::m_flFallbackWear, 0.1f);

					if (shouldUpdate)
						mem.Write<std::int32_t>(mem.Read<std::uintptr_t>(globals::engineAddress + offsets::dwClientState) + 0x174, -1);
				}
			}
		}

	}
}

void hacks::FlashThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::Flash)
		{
			int flashDur = 0;

			const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			if (localPlayer == NULL)
				while (localPlayer == NULL)
					const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			flashDur = mem.Read<int>(localPlayer + offsets::m_flFlashDuration);
			if (flashDur > 0)
				mem.Write<int>(localPlayer + offsets::m_flFlashDuration, 0);
		}
	}
}

void hacks::FlashPerThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::LessFlash)
		{
			const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			if (localPlayer == NULL)
				while (localPlayer == NULL)
					const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			if (mem.Read<float>(localPlayer + offsets::m_flFlashMaxAlpha) > 75.f)
			{
				//cout << mem->Read<float>(localPlayer + m_flFlashMaxAlpha) << endl;
				mem.Write<float>(localPlayer + offsets::m_flFlashMaxAlpha, globals::FlashPer[0]);
			}
		}
	}
}


void hacks::spectatorList(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2));

		if (globals::spec)
		{
			auto localPlayer = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			globals::hasSpectators = false;

			for (int i = 1; i < 32; i++) // loops through all the players
			{
				auto entity = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

				if (!entity || entity == localPlayer) continue;

				auto localPlayeridx = mem.Read<int>(localPlayer + 0x64);

				auto ObserverTarget = mem.Read<int>(entity + offsets::m_hObserverTarget) & 0xFFF;

				if (ObserverTarget == localPlayeridx) 
					//std::cout << entity << ": Is Spectating you!" << std::endl;
					globals::hasSpectators = true;
				else
					globals::hasSpectators = false;
			}
		}
	}
}

void hacks::TriggerbotThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::Triggerbot)
		{
			if (!GetAsyncKeyState(globals::TriggerKey))
				continue;

			const auto& localPlayer = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
			const auto& localHealth = mem.Read<std::int32_t>(localPlayer + offsets::m_iHealth);

			if (!localHealth)
				continue;

			const auto& crosshairId = mem.Read<std::int32_t>(localPlayer + offsets::m_iCrosshairId);

			if (!crosshairId || crosshairId > 64)
				continue;

			const auto& player = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList + (crosshairId - 1) * 0x10);

			if (!mem.Read<std::int32_t>(player + offsets::m_iHealth))
				continue;

			if (mem.Read<std::int32_t>(player + offsets::m_iTeamNum) == mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum))
				continue;

			mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceAttack, 6);
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceAttack, 4);

		}

	}
}

void hacks::RadarThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::Radar)
		{
			const auto& localPlayer = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			const auto& localTeam = mem.Read<uintptr_t>(globals::clientAddress + offsets::m_iTeamNum);

			for (auto i = 1; i <= 64; ++i)
			{
				const auto& entity = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

				if (mem.Read<uintptr_t>(entity + offsets::m_iTeamNum) == localTeam)
					continue;

				mem.Write<bool>(entity + offsets::m_bSpotted, true);

			}

		}

	}
}


void hacks::ESPoverlay(const Memory& mem) noexcept
{
	static ViewMatrix vm;
	static uintptr_t local_player;

	if (!local_player) {
		local_player = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
	}

	if (local_player) {
		const auto entity_list = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList);

		for (int i = 1; i <= 64; i++) {
			const auto entity = mem.Read<uintptr_t>(entity_list + i * 0x10);

			if (!entity) {
				continue;
			}

			const auto life_state = mem.Read<int>(entity + offsets::m_lifeState);

			if (life_state != 0) {
				continue;
			}

			const auto dormant = mem.Read<bool>(entity + offsets::m_bDormant);

			if (dormant) {
				continue;
			}

			const auto team_num = mem.Read<int>(entity + offsets::m_iTeamNum);
			const auto origin = mem.Read<Vector>(entity + offsets::m_vecOrigin);
			const auto bone_matrix = mem.Read<uintptr_t>(entity + offsets::m_dwBoneMatrix);

			Vector world_to_screen;

			if (world_to_screen(origin, screen_pos, vm)) {
				ImGui::GetOverlayDrawList()->AddText(ImVec2(screen_pos.x, screen_pos.y), team_num == 2 ? ImColor(255, 0, 0) : ImColor(0, 0, 255), "Player");
			}
		}

		const auto view_matrix_ptr = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwViewMatrix);

		if (view_matrix_ptr) {
			vm = mem.Read<ViewMatrix>(view_matrix_ptr);
		}
	}

	while (!should_close) {
		MSG message;

		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		if (GetAsyncKeyState(VK_END) & 1) {
			should_close = true;
		}

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Overlay");

		render_overlay();

		ImGui::End();

		ImGui::EndFrame();
		ImGui::Render();
		directx_device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0);

		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		directx_device->Present(nullptr, nullptr, nullptr, nullptr);
	}

}



/*void hacks::BoxESP(const Memory& mem) noexcept
{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		ShowWindow(window, cmd_show);
		UpdateWindow(window);

		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX11_Init(device, device_context);

		bool running = true;
		while (running) {
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				if (msg.message == WM_QUIT) {
					running = false;
				}
			}

			if (!running) {
				break;
			}

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			const auto local_player = mem::Read<DWORD>(handle, client + offsets::local_player);

			if (local_player) {
				const auto local_team = mem::Read<int>(handle, local_player + offsets::team_num);
				const auto view_matrix = mem::Read<ViewMatrix>(handle, client + offsets::view_matrix);

				for (int i = 1; i < 32; ++i) {
					const auto player = mem::Read<DWORD>(handle, client + offsets::entity_list + i * 0x10);

					if (!player) {
						continue;
					}

					if (mem::Read<bool>(handle, player + offsets::dormant)) {
						continue;
					}

					if (mem::Read<int>(handle, player + offsets::team_num) == local_team) {
						continue;
					}

					if (mem::Read<int>(handle, player + offsets::life_state) != 0) {
						continue;
					}

					const auto bones = mem::Read<DWORD>(handle, player + offsets::bone_matrix);

					if (!bones) {
						continue;
					}

					Vector head_pos{
						mem::Read<float>(handle, bones + 0x30 * 8 + 0x0C),
						mem::Read<float>(handle, bones + 0x30 * 8 + 0x1C),
						mem::Read<float>(handle, bones + 0x30 * 8 + 0x2C)
					};

					auto feet_pos = mem::Read<Vector>(handle, player + offsets::origin);

					Vector top;
					Vector bottom;
					if (world_to_screen(head_pos + Vector{ 0, 0, 11.f }, top, view_matrix) && world_to_screen(feet_pos - Vector{ 0, 0, 7.f }, bottom, view_matrix)) {
						const float h = bottom.y - top.y;
						const float w = h * 0.35f;

						ImGui::GetBackgroundDrawList()->AddRect({ top.x - w, top.y }, { top.x + w, bottom.y }, ImColor(1.f, 1.f, 1.f));
					}
				}
			}

			ImGui::Render();

			constexpr float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
			device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
			device_context->ClearRenderTargetView(render_target_view, clear_color);

			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

			swap_chain->Present(1U, 0U);
		}
}
*/



/*void hacks::FOVThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (globals::gameFOV)
		{
			const auto& localPlayer = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			mem.Write(localPlayer + offsets::m_iFOV, globals::gameFOVslider);
		}
		else
		{
			const auto& localPlayer = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			mem.Write(localPlayer + offsets::m_iFOV, 90.0);
		}
	}
}*/

/*void hacks::spectatorList2(const Memory& mem) noexcept
{
	bool isActive = false;

	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2));

		CGlobalVars GlobalVars = mem.Read<CGlobalVars>(globals::engineAddress + offsets::dwGlobalVars);

		const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

		if (globals::spec)
		{
			ImGui::Begin("Spectators", &isActive, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs);
			{
				for (int i = 0; i < GlobalVars.maxClients; i++)
				{
					const auto entity = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

					int LocalIndex = mem.Read<int>(localPlayer + 0x64);

					int ObserverTarget = mem.Read<int>(entity + offsets::m_hObserverTarget) & 0xFFF;

					if (ObserverTarget == LocalIndex)
						ImGui::Text("%s", GetPlayerName(i, mem));
				}
			}
		}
	}
}*/


/*

 spec list
    if (!localPlayer || !localPlayer->isAlive())
return;

interfaces->surface->setTextFont(Surface::font);

if (config->misc.spectatorList.rainbow)
interfaces->surface->setTextColor(rainbowColor(memory->globalVars->realtime, config->misc.spectatorList.rainbowSpeed));
else
interfaces->surface->setTextColor(config->misc.spectatorList.color);

const auto [width, height] = interfaces->surface->getScreenSize();

auto textPositionY = static_cast<int>(0.5f * height);

for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
	const auto entity = interfaces->entityList->getEntity(i);
	if (!entity || entity->isDormant() || entity->isAlive() || entity->getObserverTarget() != localPlayer.get())
		continue;

	PlayerInfo playerInfo;

	if (!interfaces->engine->getPlayerInfo(i, playerInfo))
		continue;

	if (wchar_t name[128]; MultiByteToWideChar(CP_UTF8, 0, playerInfo.name, -1, name, 128)) {
		const auto [textWidth, textHeight] = interfaces->surface->getTextSize(Surface::font, name);
		interfaces->surface->setTextPosition(width - textWidth - 5, textPositionY);
	textPositionY -= textHeight;
		interfaces->surface->printText(name);
	}
}
}

void Aim::recoil(Vector2& oldPunch) {
	// Offset based vars
	int shotsFired = MemMan.ReadMem<int>(val.localPlayer + offset.m_iShotsFired);
	uintptr_t clientState = MemMan.ReadMem<uintptr_t>(val.engineModule + offset.dwClientState);
	Vector2 clientStateViewAngles = MemMan.ReadMem<Vector2>(clientState + offset.dwClientState_ViewAngles);
	Vector2 aimPunch = MemMan.ReadMem<Vector2>(val.localPlayer + offset.m_aimPunchAngle);

	// Calculated vars
	Vector2 recoilVec{
	clientStateViewAngles.x + oldPunch.x - aimPunch.x * 2.f,
	clientStateViewAngles.y + oldPunch.y - aimPunch.y * 2.f
	};

	// Error check
	if (recoilVec.x > 89.f) {
		recoilVec.x = 89.f;
	}
	if (recoilVec.x < -89.f) {
		recoilVec.x = -89.f;
	}
	while (recoilVec.y > 180.f) {
		recoilVec.y -= 360.f;
	}
	while (recoilVec.y < -180.f) {
		recoilVec.y += 360.f;
	}
	// Write to mem
	MemMan.WriteMem<Vector2>(clientState + offset.dwClientState_ViewAngles, recoilVec);
	oldPunch.x = aimPunch.x * 2.f;
	oldPunch.y = aimPunch.y * 2.f;
}
*/