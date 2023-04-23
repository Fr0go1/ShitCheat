#pragma once
#include "memory.h"

namespace hacks
{
	// run visual hacks
	void VisualsThread(const Memory& mem) noexcept;
	void AimbotThread(const Memory& mem) noexcept;
	void BhopThread(const Memory& mem) noexcept;
	void ChamsThread(const Memory& mem) noexcept;
	void SkinThread(const Memory& mem) noexcept;
	void FlashThread(const Memory& mem) noexcept;
	void FlashPerThread(const Memory& mem) noexcept;
	void spectatorList(const Memory& mem) noexcept;
	/*void spectatorList2(const Memory& mem) noexcept; */
	void RadarThread(const Memory& mem) noexcept;
	void TriggerbotThread(const Memory& mem) noexcept;
	void ESPoverlay(const Memory& mem) noexcept;
	//void FOVThread(const Memory& mem) noexcept;
}
