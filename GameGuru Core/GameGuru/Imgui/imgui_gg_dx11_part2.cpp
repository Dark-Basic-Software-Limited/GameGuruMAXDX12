//################################################################
//#### PE: ImgBtn                                             ####
//#### Used to add image buttons directly from a GG image id. ####
//################################################################

#include "CImageC.h"

namespace ImGui {

	bool MenuItem2(const char* label, const char* shortcut, bool selected, bool enabled)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		ImGuiStyle& style = g.Style;
		ImVec2 pos = window->DC.CursorPos;
		ImVec2 label_size = CalcTextSize(label, NULL, true);

		// We've been using the equivalent of ImGuiSelectableFlags_SetNavIdOnHover on all Selectable() since early Nav system days (commit 43ee5d73),
		// but I am unsure whether this should be kept at all. For now moved it to be an opt-in feature used by menus only.
		ImGuiSelectableFlags flags = ImGuiSelectableFlags_PressedOnRelease | ImGuiSelectableFlags_SetNavIdOnHover | (enabled ? 0 : ImGuiSelectableFlags_Disabled);
		bool pressed;
		if (window->DC.LayoutType == ImGuiLayoutType_Horizontal)
		{
			// Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
			// Note that in this situation we render neither the shortcut neither the selected tick mark
			float w = label_size.x;
			window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * 0.5f);
			PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x * 2.0f, style.ItemSpacing.y));
			pressed = Selectable(label, selected, flags, ImVec2(w, 0.0f));
			PopStyleVar();
			window->DC.CursorPos.x += (float)(int)(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
		}
		else
		{
			ImVec2 shortcut_size = shortcut ? CalcTextSize(shortcut, NULL) : ImVec2(0.0f, 0.0f);
			float w = window->MenuColumns.DeclColumns(label_size.x, shortcut_size.x, (float)(int)(g.FontSize * 1.20f)); // Feedback for next frame
			float extra_w = ImMax(0.0f, GetContentRegionAvail().x - w);
			pressed = Selectable(label, selected, flags | ImGuiSelectableFlags_DrawFillAvailWidth, ImVec2(w, 0.0f));
			if (shortcut_size.x > 0.0f)
			{
				PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
				RenderText(pos + ImVec2(window->MenuColumns.Pos[1] + extra_w, 0.0f), shortcut, NULL, false);
				PopStyleColor();
			}
			if (selected)
				RenderCheckMark(pos + ImVec2(window->MenuColumns.Pos[2] + extra_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), g.FontSize * 0.866f);
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
		return pressed;
	}

	const char* CalcWordWrapPositionB(float scale, const char* textorig, const char* text_end, float wrap_width, float line_start)
	{
		// Simple word-wrapping for English, not full-featured. Please submit failing cases!
		// FIXME: Much possible improvements (don't cut things like "word !", "word!!!" but cut within "word,,,,", more sensible support for punctuations, support for Unicode punctuations, etc.)

		// For references, possible wrap point marked with ^
		//  "aaa bbb, ccc,ddd. eee   fff. ggg!"
		//      ^    ^    ^   ^   ^__    ^    ^

		// List of hardcoded separators: .,;!?'"

		// Skip extra blanks after a line returns (that includes not counting them in width computation)
		// e.g. "Hello    world" --> "Hello" "World"

		// Cut words that cannot possibly fit within one line.
		// e.g.: "The tropical fish" with ~5 characters worth of width --> "The tr" "opical" "fish"

		float line_width = line_start / scale;
		float word_width = 0.0f;
		float blank_width = 0.0f;
		wrap_width /= scale; // We work with unscaled widths to avoid scaling every characters

		// can mess up with text starts with spaces!
		const char* text = textorig;
		while ((*text == ' ' || *text == '/t') && text<text_end) text++;

		const char* word_end = text;
		//const char* prev_word_end = NULL;
		const char* prev_word_end = line_start > 0.0f ? word_end : NULL;
		bool inside_word = true;

		const char* s = text;
		while (s < text_end)
		{
			unsigned int c = (unsigned int)*s;
			const char* next_s;
			if (c < 0x80)
				next_s = s + 1;
			else
				next_s = s + ImTextCharFromUtf8(&c, s, text_end);
			if (c == 0)
				break;

			if (c < 32)
			{
				if (c == '\n')
				{
					line_width = word_width = blank_width = 0.0f;
					inside_word = true;
					s = next_s;
					continue;
				}
				if (c == '\r')
				{
					s = next_s;
					continue;
				}
			}

			const float char_width = ImGui::GetFont()->GetCharAdvance(c); //((int)c < IndexAdvanceX.Size ? IndexAdvanceX.Data[c] : FallbackAdvanceX);
			if (ImCharIsBlankW(c))
			{
				if (inside_word)
				{
					line_width += blank_width;
					blank_width = 0.0f;
					word_end = s;
				}
				blank_width += char_width;
				inside_word = false;
			}
			else
			{
				word_width += char_width;
				if (inside_word)
				{
					word_end = next_s;
				}
				else
				{
					prev_word_end = word_end;
					line_width += word_width + blank_width;
					word_width = blank_width = 0.0f;
				}

				// Allow wrapping after punctuation.
				inside_word = !(c == '.' || c == ',' || c == ';' || c == '!' || c == '?' || c == '\"');
			}

			// We ignore blank width at the end of the line (they can be skipped)
			if (line_width + word_width > wrap_width)
			{
				// Words that cannot possibly fit within an entire line will be cut anywhere.
				if (word_width < wrap_width)
					s = prev_word_end ? prev_word_end : word_end;
				break;
			}

			s = next_s;
		}

		return s;
	}


	void TextCenter(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		const char* text_end = g.TempBuffer + ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

		
		float textwidth = ImGui::CalcTextSize(g.TempBuffer).x;
		//PE: Change - Account for indent.
		ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2( (GetContentRegionAvail().x*0.5) - (textwidth*0.5) - (window->DC.Indent.x*0.5) , 0.0f));

		TextEx(g.TempBuffer, text_end, ImGuiTextFlags_NoWidthForLargeClippedText);
		va_end(args);
	}

	bool StyleCollapsingHeader(const char* label, ImGuiTreeNodeFlags flags)
	{
		if (pref.current_style == 25)
		{
			//ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, ImVec2(0, 0));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 1.00f));
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 0.0f;
		}

		bool bret = CollapsingHeader(label, flags);
		
		if (pref.current_style == 25)
		{
			ImGui::PopStyleColor();
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 1.0f;
			//ImGui::PopStyleVar();
		}

		return bret;
	}

	bool HyberlinkButton(const char* label, const ImVec2& size_arg)
	{
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 0.00f)); //no border.
		auto *style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;
		
		ImVec4 backImGuiCol_Button = colors[ImGuiCol_Button];
		ImVec4 backImGuiCol_ButtonHovered = colors[ImGuiCol_ButtonHovered];
		ImVec4 backImGuiCol_ButtonActive = colors[ImGuiCol_ButtonActive];

		colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

		style->FrameBorderSize = 0.0f;

		ImVec2 cpos = ImGui::GetCursorPos();

		bool bret = ImGui::Button(label, size_arg);
		
		ImGuiWindow* window = GetCurrentWindow();

		//window->DC.CursorPosPrevLine.x
		float yoffset = 8.0;
		float xpadding = 4.0;
		ImRect image_bb(ImVec2(window->Pos.x + window->Scroll.x + cpos.x + xpadding, window->DC.CursorPos.y - yoffset), ImVec2(window->DC.CursorPosPrevLine.x- xpadding, (window->DC.CursorPos.y - yoffset) + 2.0f));

		ImVec4 col = backImGuiCol_Button;
		if (IsItemHovered())
		{
			col = backImGuiCol_ButtonHovered;
		}
		window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(col));

		ImGui::PopStyleColor();
		style->FrameBorderSize = 1.0f;

		colors[ImGuiCol_Button] = backImGuiCol_Button;
		colors[ImGuiCol_ButtonHovered] = backImGuiCol_ButtonHovered;
		colors[ImGuiCol_ButtonActive] = backImGuiCol_ButtonActive;

		return bret;
	}

	bool MinMaxButtonEx(const char* str_id, ImGuiDir dir)
	{
		float sz = GetFrameHeight();
		ImVec2 size = ImVec2(sz, sz);

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiID id = window->GetID(str_id);
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
		const float default_size = GetFrameHeight();
		ItemSize(size, (size.y >= default_size) ? g.Style.FramePadding.y : 0.0f);
		if (!ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, 0);

		// Render
		const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
		const ImU32 text_col = GetColorU32(ImGuiCol_Text);
		RenderNavHighlight(bb, id);
		RenderFrame(bb.Min, bb.Max, bg_col, true, g.Style.FrameRounding);
		RenderArrowOutLine(window->DrawList, bb.Min + ImVec2(ImMax(0.0f, (size.x - g.FontSize) * 0.5f), ImMax(0.0f, (size.y - g.FontSize) * 0.5f)), text_col, dir , 1.0f);

		return pressed;
	}

	void AddTriangleNotClosed(ImDrawList* draw_list,const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, ImU32 col, float thickness)
	{
		if ((col & IM_COL32_A_MASK) == 0)
			return;

		draw_list->PathLineTo(p2);
		draw_list->PathLineTo(p1);
		draw_list->PathLineTo(p3);
		draw_list->PathStroke(col, false, thickness);
	}

	void RenderArrowOutLine(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float scale)
	{
		const float h = draw_list->_Data->FontSize * 1.00f;
		float r = h * 0.40f * scale;
		ImVec2 center = pos + ImVec2(h * 0.50f, h * 0.50f * scale);

		ImVec2 a, b, c;
		switch (dir)
		{
		case ImGuiDir_Up:
		case ImGuiDir_Down:
			if (dir == ImGuiDir_Up) r = -r;
			a = ImVec2(+0.000f, +0.750f) * r;
			b = ImVec2(-0.866f, -0.750f) * r;
			c = ImVec2(+0.866f, -0.750f) * r;
			break;
		case ImGuiDir_Left:
		case ImGuiDir_Right:
			if (dir == ImGuiDir_Left) r = -r;
			a = ImVec2(+0.750f, +0.000f) * r;
			b = ImVec2(-0.750f, +0.866f) * r;
			c = ImVec2(-0.750f, -0.866f) * r;
			break;
		case ImGuiDir_None:
		case ImGuiDir_COUNT:
			IM_ASSERT(0);
			break;
		}
		AddTriangleNotClosed(draw_list,center + a, center + b, center + c, col,1.75f);
	}

	bool StyleButtonEx(const char* label, const ImVec2& size_arg, bool bDisabled)
	{
		ImGuiButtonFlags_ flags = ImGuiButtonFlags_None;
		if (bDisabled == true) flags = ImGuiButtonFlags_Disabled;
		if (pref.current_style == 25)
		{
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 1.00f));
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 0.0f;
		}
		if (bDisabled == true) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 0.5f));
		bool bret = ImGui::ButtonEx(label, size_arg, flags);
		if (bDisabled == true) ImGui::PopStyleColor();
		if (pref.current_style == 25)
		{
			ImGui::PopStyleColor();
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 1.0f;
		}
		return bret;
	}

	bool StyleButton(const char* label, const ImVec2& size_arg)
	{
		if (pref.current_style == 25)
		{
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.43f, 0.57f, 1.00f));
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 0.0f;
		}
		bool bret = ImGui::Button(label, size_arg);
		if (pref.current_style == 25)
		{
			ImGui::PopStyleColor();
			auto *style = &ImGui::GetStyle();
			style->FrameBorderSize = 1.0f;
			//ImGui::PopStyleVar();
		}
		return bret;
	}

	bool StyleButtonDark(const char* label, const ImVec2& size_arg)
	{
		auto* style = &ImGui::GetStyle();
		style->FrameBorderSize = 0.0f;
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.33f, 0.5f, 1.0f));
		bool bret = ImGui::Button(label, size_arg);
		ImGui::PopStyleColor();
		style->FrameBorderSize = 1.0f;
	
		return bret;
	}

	//PE: Max gadget types.
	bool bLastSliderHovered = false;
	bool IsLastSliderHovered(void)
	{
		return(bLastSliderHovered);
	}

	bool MaxSliderInputInt(const char* label, int* v, int v_min, int v_max, const char* tooltip, int boxWidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{
			if (v_max == g_SliderData[i].fDefaultMaxValue)
			{
				if (strcmp(cUniqueLabel, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;

					break;
				}
			}
			
			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, cUniqueLabel);
				g_SliderData.push_back(data);

				v_max = *v;
			}
		}

		int iInput = *v;
		bool bRet = false;
		ImGui::PushItemWidth(-10 - boxWidth - 10);

		if (ImGui::SliderInt(cUniqueLabel, &iInput, v_min, v_max, " "))
		{
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered()) {
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PushItemWidth(boxWidth);
		
		if (strstr(cUniqueLabel, "##"))
			strcat(cUniqueLabel, "i");
		else
			strcat(cUniqueLabel, "##i");

		if (ImGui::InputInt(cUniqueLabel, &iInput, 0, 0))
		{

			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered()) {
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		if (iInput < v_min)
			iInput = v_min;

		*v = iInput;
		return(bRet);
	}

	bool MaxSliderInputFloat(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{
			// Compare the default values first, to save doing lots of string compares.
			if (v_max == g_SliderData[i].fDefaultMaxValue)  
			{
				if (strcmp(label, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;
					maxval = v_max;

					break;
				}
			}

			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, label);
				g_SliderData.push_back(data);

				v_max = *v;
				maxval = v_max;
			}
		}

		float fInput = *v - v_min;
		
		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iMaxInt = 100;
		ImGui::PushItemWidth(-10 - 40);
		if(v_max <= 10.0)
			iControlInput = maxval / fRange * (fInput + 0.009f); // Added +0.009 to eliminate floating point error which resulted in iControlInput dropping 1
		else
			iControlInput = maxval / fRange * ceil(fInput);

		if (iControlInput < startval)
			iControlInput = startval;

		iMaxInt = maxval;

		if (ImGui::SliderInt (cUniqueLabel, &iControlInput, startval, iMaxInt, " "))
		{
			fInput = (float)iControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");

		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			if (v_max <= 10.0)
				fInput = (float)iControlInput * (fRange / maxval);
			else
				fInput = ceil((float)iControlInput * (fRange / maxval));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	bool MaxSliderInputFloat2(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{
			// Compare the default values first, to save doing lots of string compares.
			if (v_max == g_SliderData[i].fDefaultMaxValue)
			{
				if (strcmp(label, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;
					maxval = v_max;

					break;
				}
			}

			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, label);
				g_SliderData.push_back(data);

				v_max = *v;
				maxval = v_max;
			}
		}

		float fInput = *v - v_min;
		float fRange = v_max - v_min;
		bool bRet = false;
		float fControlInput, iMaxInt = 100;
		ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
		fControlInput = maxval / fRange * fInput;
		if (fControlInput < startval)
			fControlInput = startval;

		iMaxInt = maxval;

		if (ImGui::SliderFloat(cUniqueLabel, &fControlInput, startval, iMaxInt, " ")) //"%.2f"
		{
			fInput = (float)fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputFloat(cUniqueLabel, &fControlInput, 0, 0, 2))
		{
			fInput = (float)fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	// Use for sliders with large ranges, where fine control over the slider values is required.
	bool MaxSliderInputFloatPower(const char* label, float* v, float v_min, float v_max, const char* tooltip, int startval, float maxval, int numericboxwidth, float power, int precision)
	{
		bLastSliderHovered = false;
		if (!label) return(false);
		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		int iCounter = g_SliderData.size() - 1;
		// Check to see if this slider has a non-default max value.
		for (int i = 0; i < g_SliderData.size(); i++)
		{ 
			// Compare the default values first, to save doing lots of string compares.
			if (v_max == g_SliderData[i].fDefaultMaxValue)
			{
				if (strcmp(label, g_SliderData[i].cID) == 0)
				{
					// This slider has a non-default max value, so use it instead of the value passed in.
					if (*v > g_SliderData[i].fHighestMaxValue)
						g_SliderData[i].fHighestMaxValue = *v;

					v_max = g_SliderData[i].fHighestMaxValue;
					maxval = v_max;

					break;
				}
			}

			iCounter--;
		}

		if (iCounter <= 0)
		{
			// This slider does not have a custom max value set yet.
			if (*v > v_max)
			{
				SliderData data;
				data.fDefaultMaxValue = v_max;
				data.fHighestMaxValue = *v;
				strcpy(data.cID, label);
				g_SliderData.push_back(data);

				v_max = *v;
				maxval = v_max;
			}
		}

		float fInput = *v - v_min;
		float fRange = v_max - v_min;
		bool bRet = false;
		float fControlInput, fMax = 100;
		ImGui::PushItemWidth(-10 - 10 - numericboxwidth);
		fControlInput = maxval / fRange * fInput;
		if (fControlInput < startval)
			fControlInput = startval;

		fMax = maxval;

		if (ImGui::SliderFloat(cUniqueLabel, &fControlInput, startval, fMax, "", power))
		{
			fInput = fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(numericboxwidth);
		strcat(cUniqueLabel, "##i");
		//if (ImGui::InputInt(cUniqueLabel, &fControlInput, 0, 0))
		if (ImGui::InputFloat(cUniqueLabel, &fControlInput, 0, 0, precision))
		{
			fInput = fControlInput * (fRange / maxval);
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		*v = v_min + fInput;
		return(bRet);
	}

	bool MaxSliderInputRangeFloat(const char* label, float* v, float* v2, float v_min, float v_max, const char* tooltip)
	{
		bLastSliderHovered = false;
		if (!label) return(false);

		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		//int iCounter = g_SliderData.size() - 1;
		//// Check to see if this slider has a non-default max value.
		//for (int i = 0; i < g_SliderData.size(); i++)
		//{
		//	// Compare the default values first, to save doing lots of string compares.
		//	if (v_max == g_SliderData[i].fDefaultMaxValue)
		//	{
		//		if (strcmp(label, g_SliderData[i].cID) == 0)
		//		{
		//			// This slider has a non-default max value, so use it instead of the value passed in.
		//			if (*v2 > g_SliderData[i].fHighestMaxValue)
		//				g_SliderData[i].fHighestMaxValue = *v2;
		//
		//			v_max = g_SliderData[i].fHighestMaxValue;
		//
		//			break;
		//		}
		//	}
		//
		//	iCounter--;
		//}
		//
		//if (iCounter <= 0)
		//{
		//	// This slider does not have a custom max value set yet.
		//	if (*v2 > v_max)
		//	{
		//		SliderData data;
		//		data.fDefaultMaxValue = v_max;
		//		data.fHighestMaxValue = *v2;
		//		strcpy(data.cID, label);
		//		g_SliderData.push_back(data);
		//
		//		v_max = *v2;
		//	}
		//}

		float fInput = *v - v_min;
		float fInput2 = *v2;

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iControlInput2;
		iControlInput = 100.0 / fRange * fInput;
		iControlInput2 = 100.0 / fRange * fInput2;
		float fTmp1 = iControlInput, fTmp2 = iControlInput2;

		//ImGui::SetCursorPos( ImVec2(ImGui::GetCursorPosX(), end_post.y) );
		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			fInput = ceil((float)iControlInput * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(-10 - 40); //77
		ImVec2 end_post = ImGui::GetCursorPos();
		strcat(cUniqueLabel, "slider");
		if (ImGui::RangeSlider(cUniqueLabel, fTmp1, fTmp2, 100.0f, false))
		{
			fInput = fTmp1 * (fRange / 100.0);
			fInput2 = fTmp2 * (fRange / 100.0);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "t");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput2, 0, 0))
		{
			fInput2 = ceil((float)iControlInput2 * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		*v = v_min + fInput;
		*v2 = fInput2;
		return(bRet);
	}


	bool MaxSliderInputRangeFloatDirect(const char* label, float* v, float* v2, float v_min, float v_max, const char* tooltip)
	{
		bLastSliderHovered = false;
		if (!label) return(false);

		char cUniqueLabel[256];
		strncpy(cUniqueLabel, label, 240);
		cUniqueLabel[240] = 0;

		float fInput = *v - v_min;
		float fInput2 = *v2;

		float fRange = v_max - v_min;
		bool bRet = false;
		int iControlInput, iControlInput2;
		iControlInput = 100.0 / fRange * fInput;
		iControlInput2 = 100.0 / fRange * fInput2;
		//float fTmp1 = iControlInput, fTmp2 = iControlInput2;
		float fTmp1 = fInput, fTmp2 = fInput2;

		//ImGui::SetCursorPos( ImVec2(ImGui::GetCursorPosX(), end_post.y) );
		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "##i");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput, 0, 0))
		{
			fInput = ceil((float)iControlInput * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(-10 - 40); //77
		ImVec2 end_post = ImGui::GetCursorPos();
		strcat(cUniqueLabel, "slider");
		if (ImGui::RangeSlider(cUniqueLabel, fTmp1, fTmp2, v_max, false))
		{
			fInput = fTmp1; // *(fRange / 100.0);
			fInput2 = fTmp2; // *(fRange / 100.0);
			bRet = true;
		}
		if (tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushItemWidth(30);
		strcat(cUniqueLabel, "t");
		if (ImGui::InputInt(cUniqueLabel, &iControlInput2, 0, 0))
		{
			fInput2 = ceil((float)iControlInput2 * (fRange / 100.0));
			bRet = true;
		}
		if (!pref.iTurnOffEditboxTooltip && tooltip && ImGui::IsItemHovered())
		{
			bLastSliderHovered = true;
			ImGui::SetTooltip(tooltip);
		}
		ImGui::PopItemWidth();

		//		if (fInput > fInput2)
		//		{
		//			float ftmp = fInput;
		//			fInput = fInput2;
		//			fInput2 = ftmp;
		//		}

		*v = v_min + fInput;
		*v2 = fInput2;
		return(bRet);
	}


	bool BeginPopupContextItemAGK(const char* str_id, int mouse_button)
	{
		ImGuiWindow* window = GImGui->CurrentWindow;
		ImGuiID id = str_id ? window->GetID(str_id) : window->DC.LastItemId; // If user hasn't passed an ID, we can use the LastItemID. Using LastItemID as a Popup ID won't conflict!
		IM_ASSERT(id != 0);                                                  // You cannot pass a NULL str_id if the last item has no identifier (e.g. a Text() item)
		if (IsMouseReleased(mouse_button) && IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
			OpenPopupEx(id);
		return BeginPopupEx(id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking);
	}


	bool windowTabVisible(void)
	{
		if (!ImGui::IsWindowDocked()) return(true);
		ImGuiWindow* window = GetCurrentWindow();
		if (!window->DockNode) return(true);
		return window->DockTabIsVisible;
	}
	int windowTabFlags(void)
	{
		//DockTabItemStatusFlags
		ImGuiWindow* window = GetCurrentWindow();
		return (int)window->DockTabItemStatusFlags;
	}
	int windowDockNodeId(void)
	{
		//DockTabItemStatusFlags
		ImGuiWindow* window = GetCurrentWindow();
		if (!window->DockNode) return(0);
		return (int)window->DockNode->ID;
	}

	bool bBlurMode = false;
	void SetBlurMode(bool blur)
	{
		bBlurMode = blur;
	}

	bool ImgBtn(int iImageID, const ImVec2& btn_size, const ImVec4& bg_col,
		const ImVec4& drawCol_normal,
		const ImVec4& drawCol_hover,
		const ImVec4& drawCol_Down, int frame_padding, int atlasindex, int atlasrows, int atlascolumns , bool nowhite , bool gratiant ,bool center_image, bool noalpha, bool useownid, bool boost25)
	{

		ID3D11ShaderResourceView* lpTexture = GetImagePointerView(iImageID);

		if (!lpTexture) return false;
		int iTexID = iImageID;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;
		ImVec2 size = ImVec2(btn_size.x, btn_size.y);
		if (btn_size.x == 0 && btn_size.y == 0) 
		{
			size.x = (float)ImageWidth(iImageID);
			size.y = (float)ImageHeight(iImageID);
		}

		ImVec2 uv0 = ImVec2(0, 0);
		ImVec2 uv1 = ImVec2(1, 1);
		if (atlasindex > 0) {
			//atlasrows
			//atlascolumns
			float asx = (float)ImageWidth(iImageID);
			float asy = (float)ImageHeight(iImageID);

			float uvratiox = 1.0f / (asx);
			float uvratioy = 1.0f / (asy);
			float imgsizex = asx / atlasrows;
			float imgsizey = asy / atlascolumns;

			int index_x = (int)fmod(atlasindex - 1, atlasrows);
			int index_y = (atlasindex - 1) / atlasrows; //atlascolumns;

			float uvborderx = uvratiox;
			float uvbordery = uvratioy;
			uvborderx *= (imgsizex / 32);
			uvbordery *= (imgsizey / 32);

			float atlasstartx = (index_x * (imgsizex)) * uvratiox + (uvborderx*0.5f);
			float atlasstarty = (index_y * (imgsizey)) * uvratioy + (uvbordery*0.5f);
			float atlassizex = (imgsizex)* uvratiox - (uvborderx); //0.987
			float atlassizey = (imgsizey)* uvratioy - (uvbordery);

			uv0 = ImVec2(atlasstartx, atlasstarty);
			uv1 = ImVec2(atlasstartx + atlassizex, atlasstarty + atlassizey);
		}
		if (atlascolumns == 9999)
		{
			uv0 = ImVec2(0, 0);
			uv1 = ImVec2(0.5, 1.0);
		}
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		if(!useownid) PushID(iTexID);
		const ImGuiID id = window->GetID("#image");
		if (!useownid) PopID();

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
		ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);

		image_bb.Floor(); // Fix Small graphical issue with the THUMBNAIL if non-exact-pixel cords was used.

		//PE: Add the background color. not really needed as most buttons are transparent.
		if (bg_col.w > 0.0f && !nowhite) {

			if (gratiant) {
				ImVec4 bg_fade = bg_col;
				for (int i = (int)image_bb.Min.y; i <= (int)image_bb.Max.y; i++)
				{
					window->DrawList->AddRectFilled(ImVec2(image_bb.Min.x, i), ImVec2(image_bb.Max.x, i + 1), GetColorU32(bg_fade));
					bg_fade = bg_fade + ImVec4(0.0055f, 0.0055f, 0.0055f, 0.0055f);
				}
			}
			else
				window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
		}

		ItemSize(bb);
		if (!ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held);

		// ZJ: Moved this to behind the image so it doesn't darken it.
		////PE: Add the background color. not really needed as most buttons are transparent.
		//if (bg_col.w > 0.0f && !nowhite) {

		//	if (gratiant) {
		//		ImVec4 bg_fade = bg_col;
		//		for (int i = (int)image_bb.Min.y; i <= (int)image_bb.Max.y; i ++ ) 
		//		{
		//			window->DrawList->AddRectFilled( ImVec2(image_bb.Min.x, i), ImVec2(image_bb.Max.x, i+1), GetColorU32(bg_fade));
		//			bg_fade = bg_fade + ImVec4(0.0055f, 0.0055f, 0.0055f, 0.0055f);
		//		}
		//	} else
		//		window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
		//}

		//if (pref.current_style != 3 && nowhite)
		if(pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback( (ImDrawCallback) 1 , NULL );

		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)3, NULL);
		}

		if (boost25)
			window->DrawList->AddCallback((ImDrawCallback)5, NULL); //new shader , boost colors.

		if(bBlurMode)
			window->DrawList->AddCallback((ImDrawCallback)6, NULL); //new shader , blur.


		window->DrawList->AddImage((ImTextureID)lpTexture, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(
			(hovered && held) ? drawCol_Down : hovered ? drawCol_hover : drawCol_normal));

		if ( pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL);
		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)4, NULL);
		}
		if (boost25)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL); //switch shader back to normal shader.

		if (bBlurMode)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL); //switch shader back to normal shader.

		if (pref.current_style == 0 && nowhite) {
			ID3D11ShaderResourceView* lpTextureRound = GetImagePointerView(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(1.0, 1.0, 1.0, 1.0);
			window->DrawList->AddImage((ImTextureID)lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 1 && nowhite) {
			ID3D11ShaderResourceView* lpTextureRound = GetImagePointerView(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(0.0, 0.0, 0.0, 1.0);
			window->DrawList->AddImage((ImTextureID)lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 2 && nowhite) {
			ID3D11ShaderResourceView* lpTextureRound = GetImagePointerView(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			window->DrawList->AddImage((ImTextureID)lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		//

		if (pressed) {
			return(true);
		}

		return(pressed);
	}
	bool ImgBtnWicked(void* pMaterial, const ImVec2& btn_size, const ImVec4& bg_col,
		const ImVec4& drawCol_normal,
		const ImVec4& drawCol_hover,
		const ImVec4& drawCol_Down, int frame_padding, int atlasindex, int atlasrows, int atlascolumns, bool nowhite, bool gratiant, bool center_image, bool noalpha, bool useownid)
	{

		ID3D11ShaderResourceView* lpTexture = (ID3D11ShaderResourceView*)wiRenderer::GetDevice()->MaterialGetSRV((void*)pMaterial);
		
		//(Texture*) pMaterial

//		pObjectMaterial->textures[MaterialComponent::BASECOLORMAP].resource->texture->desc.Width;

//		auto internal_state = to_internal(resource);
//		ID3D11ShaderResourceView* SRV;

		//ID3D11ShaderResourceView* lpTexture = GetImagePointerView(iImageID);

		if( (void*) lpTexture == (void*) 1)  return false;
		if (!lpTexture) return false;
		int iTexID = (int) lpTexture; //Just need to be unique for each image.

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;
		ImVec2 size = ImVec2(btn_size.x, btn_size.y);
		if (btn_size.x == 0 && btn_size.y == 0)
		{
			size.x = (float) WickedCall_GetTextureWidth(pMaterial);
			size.y = (float) WickedCall_GetTextureHeight(pMaterial);
		}

		ImVec2 uv0 = ImVec2(0, 0);
		ImVec2 uv1 = ImVec2(1, 1);
		if (atlasindex > 0) {
			//atlasrows
			//atlascolumns
			float asx = (float) WickedCall_GetTextureWidth(pMaterial);
			float asy = (float) WickedCall_GetTextureHeight(pMaterial);

			float uvratiox = 1.0f / (asx);
			float uvratioy = 1.0f / (asy);
			float imgsizex = asx / atlasrows;
			float imgsizey = asy / atlascolumns;

			int index_x = (int)fmod(atlasindex - 1, atlasrows);
			int index_y = (atlasindex - 1) / atlasrows; //atlascolumns;

			float uvborderx = uvratiox;
			float uvbordery = uvratioy;
			uvborderx *= (imgsizex / 32);
			uvbordery *= (imgsizey / 32);

			float atlasstartx = (index_x * (imgsizex)) * uvratiox + (uvborderx*0.5f);
			float atlasstarty = (index_y * (imgsizey)) * uvratioy + (uvbordery*0.5f);
			float atlassizex = (imgsizex)* uvratiox - (uvborderx); //0.987
			float atlassizey = (imgsizey)* uvratioy - (uvbordery);

			uv0 = ImVec2(atlasstartx, atlasstarty);
			uv1 = ImVec2(atlasstartx + atlassizex, atlasstarty + atlassizey);
		}
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		if (!useownid) PushID(iTexID);
		const ImGuiID id = window->GetID("#image");
		if (!useownid) PopID();

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
		ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);

		image_bb.Floor(); // Fix Small graphical issue with the THUMBNAIL if non-exact-pixel cords was used.

		ItemSize(bb);
		if (!ItemAdd(bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held);

		//PE: Add the background color. not really needed as most buttons are transparent.
		if (bg_col.w > 0.0f && !nowhite) {

			if (gratiant) {
				ImVec4 bg_fade = bg_col;
				for (int i = (int)image_bb.Min.y; i <= (int)image_bb.Max.y; i++)
				{
					window->DrawList->AddRectFilled(ImVec2(image_bb.Min.x, i), ImVec2(image_bb.Max.x, i + 1), GetColorU32(bg_fade));
					bg_fade = bg_fade + ImVec4(0.0055f, 0.0055f, 0.0055f, 0.0055f);
				}
			}
			else
				window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));
		}

		//if (pref.current_style != 3 && nowhite)
		if (pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback((ImDrawCallback)1, NULL);

		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)3, NULL);
		}

		window->DrawList->AddImage((ImTextureID)lpTexture, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(
			(hovered && held) ? drawCol_Down : hovered ? drawCol_hover : drawCol_normal));

		if (pref.current_style > 2 && pref.current_style < 10 && nowhite)
			window->DrawList->AddCallback((ImDrawCallback)2, NULL);
		if (noalpha) {
			window->DrawList->AddCallback((ImDrawCallback)4, NULL);
		}

		if (pref.current_style == 0 && nowhite) {
			ID3D11ShaderResourceView* lpTextureRound = GetImagePointerView(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(1.0, 1.0, 1.0, 1.0);
			window->DrawList->AddImage((ImTextureID)lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 1 && nowhite) {
			ID3D11ShaderResourceView* lpTextureRound = GetImagePointerView(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImVec4(0.0, 0.0, 0.0, 1.0);
			window->DrawList->AddImage((ImTextureID)lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		if (pref.current_style == 2 && nowhite) {
			ID3D11ShaderResourceView* lpTextureRound = GetImagePointerView(ROUNDING_OVERLAY); //ROUNDING_OVERLAY
			ImVec4 back = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
			window->DrawList->AddImage((ImTextureID)lpTextureRound, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(back));
		}
		//

		if (pressed) {
			return(true);
		}

		return(pressed);
	}

	struct ImGuiViewportDataWin32
	{
		HWND    Hwnd;
		bool    HwndOwned;
		DWORD   DwStyle;
		DWORD   DwExStyle;

		ImGuiViewportDataWin32() { Hwnd = NULL; HwndOwned = false;  DwStyle = DwExStyle = 0; }
		~ImGuiViewportDataWin32() { IM_ASSERT(Hwnd == NULL); }
	};

	//PE: function needed when we do test game.
	void ImGui_GG_HideWindow(ImGuiViewport* viewport)
	{
		ImGuiViewportDataWin32* data = (ImGuiViewportDataWin32*)viewport->PlatformUserData;
		if (data)
		{
			IM_ASSERT(data->Hwnd != 0);
			if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
				::ShowWindow(data->Hwnd, SW_HIDE);
			else
				::ShowWindow(data->Hwnd, SW_HIDE);
		}
	}
	void ImGui_GG_ShowWindow(ImGuiViewport* viewport)
	{
		ImGuiViewportDataWin32* data = (ImGuiViewportDataWin32*)viewport->PlatformUserData;
		if (data)
		{
			IM_ASSERT(data->Hwnd != 0);
			if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
				::ShowWindow(data->Hwnd, SW_SHOWNA);
			else
				::ShowWindow(data->Hwnd, SW_SHOW);
		}
	}

	void HideAllViewPortWindows(void)
	{
		ImGuiContext& g = *GImGui;
		ImGuiViewport* main_viewport = GetMainViewport();
		for (int i = 0; i < g.Viewports.Size; i++)
			//PE: not main viewport.
			if (main_viewport != g.Viewports[i]) {
				ImGui_GG_HideWindow(g.Viewports[i]);
			}

	}
	void ShowAllViewPortWindows(void)
	{
		ImGuiContext& g = *GImGui;
		ImGuiViewport* main_viewport = GetMainViewport();
		for (int i = 0; i < g.Viewports.Size; i++)
			//PE: not main viewport.
			if (main_viewport != g.Viewports[i]) {
				ImGui_GG_ShowWindow(g.Viewports[i]);
			}

	}

	void ToggleButton(const char* str_id, bool* v)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec4 *style_colors = ImGui::GetStyle().Colors;

		float height = ImGui::GetFrameHeight() * 0.8f;
		float bordery = ImGui::GetFrameHeight() * 0.12f;
		float width = height * 1.7f;
		float radius = height * 0.50f;

		p.y += bordery;

		ImGui::InvisibleButton(str_id, ImVec2(width, height));
		if (ImGui::IsItemClicked())
			*v = !*v;

		float t = *v ? 1.0f : 0.0f;

		ImGuiContext& g = *GImGui;
		float ANIM_SPEED = 0.08f;
		if (g.LastActiveId == g.CurrentWindow->GetID(str_id))// && g.LastActiveIdTimer < ANIM_SPEED)
		{
			float t_anim = ImSaturate(g.LastActiveIdTimer / ANIM_SPEED);
			t = *v ? (t_anim) : (1.0f - t_anim);
		}

		ImU32 col_bg;
		if (ImGui::IsItemHovered())
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_ButtonHovered]), ImVec4(style_colors[ImGuiCol_PlotHistogram]), t));
		else
			col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_FrameBg]), ImVec4(style_colors[ImGuiCol_PlotHistogram]), t));

		draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
		draw_list->AddCircleFilled(ImVec2(p.x + radius + t * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
	}

	bool RoundButton(const char* str_id, ImVec2 size, float radius)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec4 *style_colors = ImGui::GetStyle().Colors;

		float height = size.y;
		float bordery = ImGui::GetFrameHeight() * 0.12f;
		float width = size.x;
		//radius *= 0.5;
		if (radius > height)
			radius = height;

		bool ret = ImGui::InvisibleButton(str_id, ImVec2(width, height));

		//p.y += bordery;
		if (height > (radius*2.0))
			p.y += ((height - (radius*2.0)) * 0.5);
		if (width > (radius*2.0))
			p.x += ((width - (radius*2.0)) * 0.5);

		ImU32 col_bg;

		if (pref.current_style == 3) {
			col_bg = ImGui::GetColorU32(ImVec4(style_colors[ImGuiCol_Button]));
			if (!ImGui::IsItemHovered())
				col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_ButtonHovered]), ImVec4(style_colors[ImGuiCol_Button]), 0.5));
		}
		else {
			col_bg = ImGui::GetColorU32(ImVec4(style_colors[ImGuiCol_PlotHistogram]));
			if (ImGui::IsItemHovered())
				col_bg = ImGui::GetColorU32(ImLerp(ImVec4(style_colors[ImGuiCol_ButtonHovered]), ImVec4(style_colors[ImGuiCol_PlotHistogram]), 0.5));
		}

		draw_list->AddCircleFilled(ImVec2(p.x + radius, p.y + radius), radius, IM_COL32(0, 0, 0, 128));
		draw_list->AddCircleFilled(ImVec2(p.x + radius, p.y + radius), radius - 1.5f, col_bg);


		return ret;

	}
	//PE: Currently only to be used for InputText , always return true if we have a blinking cursor.
	bool MaxIsItemFocused(void)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		//PE: Check if clicked outside item.
		if (g.ActiveId != g.NavId)
			return false;

		//PE: dont consider g.NavDisableHighlight.
		if (g.NavId == 0 || g.NavId != window->DC.LastItemId)
			return false;

		// Special handling for the dummy item after Begin() which represent the title bar or tab. 
		// When the window is collapsed (SkipItems==true) that last item will never be overwritten so we need to detect the case.
		if (window->DC.LastItemId == window->ID && window->WriteAccessed)
			return false;

		return true;
	}


	int rotation_start_index;
	void ImRotateStart()
	{
		rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
	}

	ImVec2 ImRotationCenter()
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

		const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); // or use _ClipRectStack?
	}

	void ImRotateEnd(float rad, ImVec2 center)
	{
		float s = sin(rad), c = cos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	}



	//PE: https://github.com/nem0/LumixEngine/blob/timeline_gui/external/imgui/imgui_user.inl#L814

	static float s_max_timeline_value = 100.0f;

	bool BeginTimeline(const char* str_id, float max_value)
	{
		s_max_timeline_value = max_value;
		return BeginChild(str_id);
	}


	static const float TIMELINE_RADIUS = 6;


	bool TimelineEvent(const char* str_id, float* values)
	{
		ImGuiWindow* win = GetCurrentWindow();
		const ImU32 inactive_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		const ImU32 active_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
		const ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonActive]); //ImGuiCol_ColumnActive
		bool changed = false;
		ImVec2 cursor_pos = win->DC.CursorPos;

		for (int i = 0; i < 2; ++i)
		{
			ImVec2 pos = cursor_pos;
			pos.x += win->Size.x * values[i] / s_max_timeline_value + TIMELINE_RADIUS;
			pos.y += TIMELINE_RADIUS;

			SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, TIMELINE_RADIUS));
			PushID(i);
			InvisibleButton(str_id, ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
			if (IsItemActive() || IsItemHovered())
			{
				ImGui::SetTooltip("%f", values[i]);
				ImVec2 a(pos.x, GetWindowContentRegionMin().y + win->Pos.y);
				ImVec2 b(pos.x, GetWindowContentRegionMax().y + win->Pos.y);
				win->DrawList->AddLine(a, b, line_color);
			}
			if (IsItemActive() && IsMouseDragging(0))
			{
				values[i] += GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
				changed = true;
			}
			PopID();
			win->DrawList->AddCircleFilled(
				pos, TIMELINE_RADIUS, IsItemActive() || IsItemHovered() ? active_color : inactive_color);
		}

		ImVec2 start = cursor_pos;
		start.x += win->Size.x * values[0] / s_max_timeline_value + 2 * TIMELINE_RADIUS;
		start.y += TIMELINE_RADIUS * 0.5f;
		ImVec2 end = start + ImVec2(win->Size.x * (values[1] - values[0]) / s_max_timeline_value - 2 * TIMELINE_RADIUS,
			TIMELINE_RADIUS);

		PushID(-1);
		SetCursorScreenPos(start);
		InvisibleButton(str_id, end - start);
		if (IsItemActive() && IsMouseDragging(0))
		{
			values[0] += GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
			values[1] += GetIO().MouseDelta.x / win->Size.x * s_max_timeline_value;
			changed = true;
		}
		PopID();

		SetCursorScreenPos(cursor_pos + ImVec2(0, GetTextLineHeightWithSpacing()));

		win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color);

		if (values[0] > values[1])
		{
			float tmp = values[0];
			values[0] = values[1];
			values[1] = tmp;
		}
		if (values[1] > s_max_timeline_value) values[1] = s_max_timeline_value;
		if (values[0] < 0) values[0] = 0;
		return changed;
	}

	static double s_time_scale = 1;
	static double s_time_offset = 0;


	bool RangeSlider(const char* str_id, float & val1, float & val2, float max_value, bool bDisplayValues)
	{
		float values[2] = { val1 , val2 };

		s_max_timeline_value = max_value;
		ImGuiWindow* win = GetCurrentWindow();
		const ImU32 inactive_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		const ImU32 active_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
		const ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_ButtonActive]);
		bool changed = false;
		ImVec2 cursor_pos = win->DC.CursorPos;
		ImVec2 vContentSize = win->Size;
		//vContentSize.x -= 60.0f;
		float fMinimumButtonSpace = 10.0f;
		vContentSize.x = CalcItemWidth() - 10.0f; //GetContentRegionAvail();
		vContentSize.y = win->Size.y;

		const ImVec2 label_size = CalcTextSize(str_id, NULL, true);
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = win->GetID(str_id);

		ImVec2 size = CalcItemSize(ImVec2(-10,0), label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
		vContentSize.y = size.y;
		const ImRect bb(cursor_pos, cursor_pos + ImVec2(vContentSize.x + fMinimumButtonSpace, vContentSize.y) );

		vContentSize.x -= fMinimumButtonSpace;

		if (IsMouseHoveringRect(bb.Min, bb.Max)) {
			//Avtive dont look so good.
			//if (IsMouseDown(0))
			//	win->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBgActive), style.FrameRounding, ImDrawCornerFlags_Bot);
			//else
			win->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBgHovered), style.FrameRounding, ImDrawCornerFlags_Bot);
		}
		else {
			win->DrawList->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_FrameBg), style.FrameRounding, ImDrawCornerFlags_Bot);
		}

		const float border_size = g.Style.FrameBorderSize;
		if (border_size > 0.0f)
		{
			win->DrawList->AddRect(bb.Min + ImVec2(1, 1), bb.Max + ImVec2(1, 1), GetColorU32(ImGuiCol_BorderShadow), style.FrameRounding, ImDrawCornerFlags_All, border_size);
			win->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(ImGuiCol_Border), style.FrameRounding, ImDrawCornerFlags_All, border_size);
		}

		bool bButInvHover = false;
		ImGuiID gad_id[2];

		//PE: Middle bar.
		int iMiddleBarY = 6,iSideBordersX = 1; //2

		iMiddleBarY = vContentSize.y - 4.0;
		ImVec2 start = cursor_pos;
		start.x += vContentSize.x * values[0] / s_max_timeline_value + iSideBordersX * TIMELINE_RADIUS;
		start.y += (vContentSize.y*0.5) - (iMiddleBarY*0.5);
		ImVec2 end = start + ImVec2((vContentSize.x * (values[1] - values[0]) / s_max_timeline_value - iSideBordersX * TIMELINE_RADIUS) + fMinimumButtonSpace - iSideBordersX, iMiddleBarY);
		win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color,0.0f);

		for (int i = 0; i < 2; ++i)
		{
			ImVec2 pos = cursor_pos;
			float ysize = size.y - (style.FramePadding.y * 2.0f);
			pos.x += vContentSize.x * values[i] / s_max_timeline_value + TIMELINE_RADIUS;
			pos.y += ysize;
			
			if (i == 1) pos.x += fMinimumButtonSpace;

			SetCursorScreenPos(pos - ImVec2(TIMELINE_RADIUS, ysize));
			//PushID(i);
			PushOverrideID(id+i);
			InvisibleButton(str_id, ImVec2(10.0, size.y ));

			gad_id[i] = win->GetID(str_id);

			ImGuiWindow* window = GetCurrentWindow();

			//PE: Disabled the hover values , as we now have a input box.
			if (bDisplayValues)
			{
				if (IsItemActive() || IsItemHovered())
				{
					ImGui::SetTooltip("%.2f", values[i]);
					bButInvHover = true;
					SetHoveredID(0);
					window->DC.LastItemStatusFlags &= ~ImGuiItemStatusFlags_HoveredRect;
				}
			}

			if (IsItemActive() && IsMouseDragging(0))
			{
				//values[i] += (GetIO().MouseDelta.x / (vContentSize.x) ) * s_max_timeline_value;

				//PE: This system works way better, more easy to slide.
				ImVec2 mousepos = ImGui::GetMousePos();
				float range = (bb.Max.x - cursor_pos.x) - (bb.Min.x - cursor_pos.x);
				float val = mousepos.x - cursor_pos.x;
				float newval = (s_max_timeline_value / range) * val;
				values[i] = newval;
				if (values[i] > max_value)
					values[i] = max_value;
				if (values[i] < 0)
					values[i] = 0;
				changed = true;
			}
			if (i == 1 && !bButInvHover)
			{
				if (IsMouseHoveringRect(bb.Min, bb.Max)) {
					SetHoveredID(window->DC.LastItemId);
					window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HoveredRect;
				}
			}
			PopID();

			pos -= ImVec2(TIMELINE_RADIUS, ysize);
			//win->DrawList->AddCircleFilled(pos, TIMELINE_RADIUS, IsItemActive() || IsItemHovered() ? active_color : inactive_color);
			if(IsItemActive())
				win->DrawList->AddRectFilled(pos, pos + ImVec2(10.0, size.y), GetColorU32(ImGuiCol_SliderGrabActive), style.GrabRounding);
			else
				win->DrawList->AddRectFilled(pos, pos + ImVec2(10.0, size.y), GetColorU32(ImGuiCol_SliderGrab), style.GrabRounding);

		}


		SetCursorScreenPos(start);
		//PushID(-1);
		if (values[1] - values[0] > 2)
		{
			PushOverrideID(id + 3);
			ImGui::SetItemAllowOverlap();
			InvisibleButton(str_id, end - start - ImVec2(2,0) );
			static float fMiddleBarMouseXDown = -1;
			if (IsItemActive())
			{
				if (ImGui::IsMouseDown(0)) // IsMouseDragging(0)
				{
					//float deltax = GetIO().MouseDelta.x / vContentSize.x * s_max_timeline_value;
					ImVec2 mousepos = ImGui::GetMousePos();
					float range = (bb.Max.x - cursor_pos.x) - (bb.Min.x - cursor_pos.x);
					float val = mousepos.x - cursor_pos.x;
					float newval = (s_max_timeline_value / range) * val;
					if (fMiddleBarMouseXDown == -1)
						fMiddleBarMouseXDown = newval;

					float deltax = newval - fMiddleBarMouseXDown;
					fMiddleBarMouseXDown = newval;
					if (values[0] + deltax >= 0 && values[1] + deltax <= (s_max_timeline_value + 1.0))
					{
						values[0] += deltax;
						values[1] += deltax;
						changed = true;
					}
				}
			}
			else {
				if (!ImGui::IsMouseDown(0))
				{
					fMiddleBarMouseXDown = -1;
				}
			}
			PopID();
		}
		//win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color,2.0f);


		if (values[0] > values[1])
		{
			//Swap active id (and gadget).
			ImGuiContext& g = *GImGui;
			if (g.ActiveId)
			{
				if (g.ActiveId == gad_id[0])
				{
					g.ActiveId = gad_id[1];
					SetActiveID(g.ActiveId, win);
				}
				else if (g.ActiveId == gad_id[1])
				{
					g.ActiveId = gad_id[0];
					SetActiveID(g.ActiveId, win);
				}
			}

			float tmp = values[0];
			values[0] = values[1];
			values[1] = tmp;
		}

		if (values[1] > s_max_timeline_value) values[1] = s_max_timeline_value;
		if (values[0] < 0) values[0] = 0;

//		if(values[1]- values[0] > 2)
//			win->DrawList->AddRectFilled(start, end, IsItemActive() || IsItemHovered() ? active_color : inactive_color, 0.0f);

		char value_buf[64];
		sprintf(value_buf, "%.2f - %.2f", values[0], values[1]);
		if(bDisplayValues)
			RenderTextClipped(bb.Min, bb.Max, value_buf, value_buf+strlen(value_buf), NULL, ImVec2(0.5f, 0.5f));

		SetCursorScreenPos(cursor_pos);
		vContentSize.x += fMinimumButtonSpace;
		vContentSize.x += fMinimumButtonSpace;
		ItemSize(vContentSize, 0);
		SetCursorScreenPos(cursor_pos + ImVec2(0, g.FontSize + g.Style.ItemSpacing.y ));

		val1 = values[0];
		val2 = values[1];
		return changed;
	}



	void EndTimeline()
	{
		ImGuiWindow* win = GetCurrentWindow();

		ImU32 color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
		ImU32 line_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Border]);
		ImU32 text_color = ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Text]);
		float rounding = GImGui->Style.ScrollbarRounding;
		ImVec2 start(GetWindowContentRegionMin().x + win->Pos.x,
			GetWindowContentRegionMax().y - GetTextLineHeightWithSpacing() + win->Pos.y);
		ImVec2 end = GetWindowContentRegionMax() + win->Pos;

		win->DrawList->AddRectFilled(start, end, color, rounding);

		const int LINE_COUNT = 5;
		const ImVec2 text_offset(0, GetTextLineHeightWithSpacing());
		for (int i = 0; i < LINE_COUNT; ++i)
		{
			ImVec2 a = GetWindowContentRegionMin() + win->Pos + ImVec2(TIMELINE_RADIUS, 0);
			a.x += i * GetWindowContentRegionWidth() / LINE_COUNT;
			ImVec2 b = a;
			b.y = start.y;
			win->DrawList->AddLine(a, b, line_color);
			char tmp[256];
			ImFormatString(tmp, sizeof(tmp), "%.2f", i * s_max_timeline_value / LINE_COUNT);
			win->DrawList->AddText(b, text_color, tmp);
		}

		EndChild();
	}

}

void myDefaultStyles(void)
{
	ImGui::GetStyle().TabRounding = 4;
	ImGui::GetStyle().ChildRounding = 3.0f;
	ImGui::GetStyle().FrameRounding = 3.0f;
	ImGui::GetStyle().ItemSpacing = ImVec2(8.0f, 4.0f);
	ImGui::GetStyle().ScrollbarRounding = 9.0f;
	ImGui::GetStyle().WindowBorderSize = 2.0f;
}

//PE: Styles from AGKS
void myDarkStyle(ImGuiStyle* dst)
{
	ImGui::StyleColorsDark();
	//Small overwrites to dark style.
	ImGuiStyle &st = ImGui::GetStyle();
	st.WindowBorderSize = 2.0f;
	st.WindowPadding = { 4.0f,4.0f };
	st.ScrollbarSize = 18.0;
	st.Colors[ImGuiCol_Separator] = { 0.16f, 0.29f, 0.48f, 0.60f };
	st.Colors[ImGuiCol_Tab] = { 0.29f, 0.29f, 0.29f, 0.86f };
	st.Colors[ImGuiCol_DockingPreview] = ImVec4(0.36f, 0.49f, 0.68f, 0.80f);
	st.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.46f, 0.59f, 0.78f, 0.90f);
	st.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.247f, 0.353f, 0.507f, 0.90f);
	st.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	st.Colors[ImGuiCol_PopupBg] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);

	st.WindowRounding = 3.0f;
	st.ChildRounding = 3.0f;
	st.FrameRounding = 3.0f;

	//st.TabBorderSize = 0.0f;
	//st.TabRounding = 8.0f;

	st.FramePadding = ImVec2(4.0f, 4.0f);

	st.Colors[ImGuiCol_Tab] = { 0.161f, 0.290f, 0.478f, 1.000f };
	st.Colors[ImGuiCol_TabUnfocused] = { 0.161f, 0.290f, 0.478f, 1.000f };
	st.Colors[ImGuiCol_TabUnfocusedActive] = { 0.200f, 0.410f, 0.680f, 1.000f };
	st.Colors[ImGuiCol_TitleBg] = { 0.160f, 0.290f, 0.480f, 1.000f };
	TintCurrentStyle();
}

void myLightStyle(ImGuiStyle* dst)
{
	ImGui::StyleColorsLight();
	//Small overwrites to light style.
	ImGuiStyle &st = ImGui::GetStyle();
	ImVec4* Colors = st.Colors;
	st.WindowBorderSize = 2.0f;
	st.WindowPadding = { 4.0f,4.0f };
	st.ScrollbarSize = 18.0;

	st.WindowRounding = 3.0f;
	st.ChildRounding = 3.0f;
	st.FrameRounding = 3.0f;

	Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.20f, 0.00f, 1.00f); //Also <h1> tags in help.
	TintCurrentStyle();
}

void myStyle(ImGuiStyle* dst)
{
	ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
	ImVec4* Colors = style->Colors;

	Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	//Colors[ImGuiCol_TextHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	//Colors[ImGuiCol_TextActive] = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
	Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
	Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
	Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	//Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
	Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

//	Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
//	Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
//	Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

	Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
	Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	//Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
	//Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	//Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
	Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f); //Also <h1> tags in help.
	Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	//Colors[ImGuiCol_TooltipBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

	style->Colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);
	TintCurrentStyle();
}

void myStyleBlue(ImGuiStyle* dst)
{
	myStyle2(dst);
	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_TextDisabled] = ImVec4(0.39f, 0.56f, 0.68f, 1.00f);
	colors[ImGuiCol_Text] = ImVec4(0.95f, 0.98f, 1.00f, 1.00f);
	//colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.16f, 0.22f, 0.97f); //Add a little transparent.
	colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.16f, 0.22f, 0.9f); //Add a little transparent.
	colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.16f, 0.22f, 1.0f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.16f, 0.22f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.58f, 0.58f, 0.58f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.23f, 0.33f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.26f, 0.35f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.98f, 1.00f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.36f, 0.48f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.11f, 0.16f, 0.23f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.61f, 0.63f, 0.69f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.61f, 0.63f, 0.69f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.61f, 0.63f, 0.69f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.16f, 0.22f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.38f, 0.51f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.43f, 0.57f, 1.00f);

	// Wicked renders first, IMGUI last, let Wicked renderings through! 
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

	colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.6f);

	colors[ImGuiCol_DragDropTarget] = ImVec4(0.58f, 0.58f, 0.58f, 0.90f);

	//PE: Darker tabs so they dont look like buttons.
	float transparent = 0.55;
	colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.16f, 0.22f, transparent);
	colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.38f, 0.51f, transparent);
	colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.43f, 0.57f, transparent);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.22f, 0.43f, 0.57f, transparent);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.43f, 0.57f, transparent);

	//ImGui::GetStyle().TabBorderSize = 1.0f;
	//ImGui::GetStyle().TabRounding = 1.0f;


	TintCurrentStyle();

}

