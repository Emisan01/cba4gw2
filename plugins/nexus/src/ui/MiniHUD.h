#pragma once
#include "../../thirdparty/imgui/imgui.h"
#include <string>

namespace cba
{
	class MiniHUD
	{
	public:
		static MiniHUD& Get();

		void Render();

	private:
		MiniHUD() = default;
		~MiniHUD() = default;
	};
}
