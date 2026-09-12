#pragma once
#include <imgui.h>

namespace cba
{
	// ── RAII ImGui Wrappers ──────────────────────────────────────────────────
	// These wrappers guarantee that every ImGui::Begin() is matched with an
	// ImGui::End(), and every Push() with a Pop(), even in the presence of
	// early returns, exceptions, or accidental layout mismatches.
	// This structurally prevents the stack underflow bugs that crashed the
	// 2026-09-12 build.

	class ScopedChild
	{
	public:
		ScopedChild(const char* str_id, const ImVec2& size = ImVec2(0, 0), bool border = false, ImGuiWindowFlags flags = 0)
		{
			m_IsOpen = ImGui::BeginChild(str_id, size, border, flags);
		}
		~ScopedChild()
		{
			// ImGui::EndChild must always be called regardless of BeginChild returning true/false
			ImGui::EndChild();
		}

		// Allow testing if the child is visible
		explicit operator bool() const { return m_IsOpen; }

	private:
		bool m_IsOpen;
	};

	class ScopedStyleColor
	{
	public:
		ScopedStyleColor(ImGuiCol idx, ImU32 col)
		{
			ImGui::PushStyleColor(idx, col);
			m_Count = 1;
		}
		ScopedStyleColor(ImGuiCol idx, const ImVec4& col)
		{
			ImGui::PushStyleColor(idx, col);
			m_Count = 1;
		}
		~ScopedStyleColor()
		{
			if (m_Count > 0)
				ImGui::PopStyleColor(m_Count);
		}

		// Disallow copy/assignment to prevent double-pop
		ScopedStyleColor(const ScopedStyleColor&) = delete;
		ScopedStyleColor& operator=(const ScopedStyleColor&) = delete;

	private:
		int m_Count;
	};

	class ScopedStyleVar
	{
	public:
		ScopedStyleVar(ImGuiStyleVar idx, float val)
		{
			ImGui::PushStyleVar(idx, val);
			m_Count = 1;
		}
		ScopedStyleVar(ImGuiStyleVar idx, const ImVec2& val)
		{
			ImGui::PushStyleVar(idx, val);
			m_Count = 1;
		}
		~ScopedStyleVar()
		{
			if (m_Count > 0)
				ImGui::PopStyleVar(m_Count);
		}

		ScopedStyleVar(const ScopedStyleVar&) = delete;
		ScopedStyleVar& operator=(const ScopedStyleVar&) = delete;

	private:
		int m_Count;
	};

	class ScopedID
	{
	public:
		ScopedID(const char* str_id)
		{
			ImGui::PushID(str_id);
		}
		ScopedID(int int_id)
		{
			ImGui::PushID(int_id);
		}
		~ScopedID()
		{
			ImGui::PopID();
		}

		ScopedID(const ScopedID&) = delete;
		ScopedID& operator=(const ScopedID&) = delete;
	};
}
