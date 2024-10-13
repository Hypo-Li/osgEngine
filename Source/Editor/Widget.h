#pragma once
#include <Engine/Core/Asset.h>
#include <Engine/Core/AssetManager.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <ThirdParty/imgui/imgui.h>
#include <ThirdParty/imgui/imgui_internal.h>

namespace xxx
{
    template <typename T>
    static Asset* AssetCombo(const char* label, Asset* previewAsset)
    {
        Asset* result = nullptr;
        refl::Class* clazz = refl::Reflection::getClass<T>();

        if (ImGui::BeginCombo(label, previewAsset->getPath().c_str()))
        {
            AssetManager::get().foreachAsset([previewAsset, clazz, &result](Asset* asset) {
                if (asset->getClass()->isDerivedFrom(clazz))
                {
                    const bool is_selected = (previewAsset == asset);
                    if (ImGui::Selectable(asset->getPath().c_str(), is_selected))
                    {
                        result = asset;
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
            });

            ImGui::EndCombo();
        }

        return result;
    }

    template <typename T>
    static T EnumCombo(const char* label, T previewValue)
    {
        T result = previewValue;
        refl::Enum* enumerate = refl::Reflection::getEnum<T>();
        std::string currentValueName(enumerate->getNameByValue(int64_t(previewValue)));
        if (ImGui::BeginCombo(label, currentValueName.c_str()))
        {
            size_t valueCount = enumerate->getValueCount();
            for (size_t i = 0; i < valueCount; ++i)
            {
                int64_t currentValue = enumerate->getValueByIndex(i);
                const bool is_selected = (currentValue == int64_t(previewValue));
                std::string valueName(enumerate->getNameByIndex(i));
                if (ImGui::Selectable(valueName.c_str(), is_selected))
                {
                    result = T(currentValue);
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
        return result;
    }
}

inline bool FileIcon(const char* label, bool isSelected, ImTextureID icon, ImVec2 size, bool hasPreview, int previewWidth, int previewHeight)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    float windowSpace = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    ImVec2 pos = window->DC.CursorPos;
    bool ret = false;

    if (ImGui::InvisibleButton(label, size))
        ret = true;

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    if (doubleClick && hovered)
        ret = true;


    float iconSize = size.y - g.FontSize * 2;
    float iconPosX = pos.x + (size.x - iconSize) / 2.0f;
    ImVec2 textSize = ImGui::CalcTextSize(label, 0, true, size.x);


    if (hovered || active || isSelected)
        window->DrawList->AddRectFilled(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[active ? ImGuiCol_HeaderActive : (isSelected ? ImGuiCol_Header : ImGuiCol_HeaderHovered)]));

    if (hasPreview) {
        ImVec2 availSize = ImVec2(size.x, iconSize);

        float scale = std::min<float>(availSize.x / previewWidth, availSize.y / previewHeight);
        availSize.x = previewWidth * scale;
        availSize.y = previewHeight * scale;

        float previewPosX = pos.x + (size.x - availSize.x) / 2.0f;
        float previewPosY = pos.y + (iconSize - availSize.y) / 2.0f;

        window->DrawList->AddImage(icon, ImVec2(previewPosX, previewPosY), ImVec2(previewPosX + availSize.x, previewPosY + availSize.y));
    }
    else
        window->DrawList->AddImage(icon, ImVec2(iconPosX, pos.y), ImVec2(iconPosX + iconSize, pos.y + iconSize));

    window->DrawList->AddText(g.Font, g.FontSize, ImVec2(pos.x + (size.x - textSize.x) / 2.0f, pos.y + iconSize + 16), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), label, 0, size.x);


    float lastButtomPos = ImGui::GetItemRectMax().x;
    float thisButtonPos = lastButtomPos + style.ItemSpacing.x + size.x; // Expected position if next button was on same line
    if (thisButtonPos < windowSpace)
        ImGui::SameLine();

    return ret;
}

inline bool fuzzy_match_recursive(const char* pattern, const char* str, int& outScore,
        const char* strBegin, uint8_t const* srcMatches, uint8_t* matches, int maxMatches,
        int nextMatch, int& recursionCount, int recursionLimit)
{
    // Count recursions
    ++recursionCount;
    if (recursionCount >= recursionLimit)
        return false;

    // Detect end of strings
    if (*pattern == '\0' || *str == '\0')
        return false;

    // Recursion params
    bool recursiveMatch = false;
    uint8_t bestRecursiveMatches[256];
    int bestRecursiveScore = 0;

    // Loop through pattern and str looking for a match
    bool first_match = true;
    while (*pattern != '\0' && *str != '\0') {

        // Found match
        if (tolower(*pattern) == tolower(*str)) {

            // Supplied matches buffer was too short
            if (nextMatch >= maxMatches)
                return false;

            // "Copy-on-Write" srcMatches into matches
            if (first_match && srcMatches) {
                memcpy(matches, srcMatches, nextMatch);
                first_match = false;
            }

            // Recursive call that "skips" this match
            uint8_t recursiveMatches[256];
            int recursiveScore;
            if (fuzzy_match_recursive(pattern, str + 1, recursiveScore, strBegin, matches, recursiveMatches, sizeof(recursiveMatches), nextMatch, recursionCount, recursionLimit)) {

                // Pick best recursive score
                if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
                    memcpy(bestRecursiveMatches, recursiveMatches, 256);
                    bestRecursiveScore = recursiveScore;
                }
                recursiveMatch = true;
            }

            // Advance
            matches[nextMatch++] = (uint8_t)(str - strBegin);
            ++pattern;
        }
        ++str;
    }

    // Determine if full pattern was matched
    bool matched = *pattern == '\0' ? true : false;

    // Calculate score
    if (matched) {
        const int sequential_bonus = 15;            // bonus for adjacent matches
        const int separator_bonus = 30;             // bonus if match occurs after a separator
        const int camel_bonus = 30;                 // bonus if match is uppercase and prev is lower
        const int first_letter_bonus = 15;          // bonus if the first letter is matched

        const int leading_letter_penalty = -5;      // penalty applied for every letter in str before the first match
        const int max_leading_letter_penalty = -15; // maximum penalty for leading letters
        const int unmatched_letter_penalty = -1;    // penalty for every letter that doesn't matter

        // Iterate str to end
        while (*str != '\0')
            ++str;

        // Initialize score
        outScore = 100;

        // Apply leading letter penalty
        int penalty = leading_letter_penalty * matches[0];
        if (penalty < max_leading_letter_penalty)
            penalty = max_leading_letter_penalty;
        outScore += penalty;

        // Apply unmatched penalty
        int unmatched = (int)(str - strBegin) - nextMatch;
        outScore += unmatched_letter_penalty * unmatched;

        // Apply ordering bonuses
        for (int i = 0; i < nextMatch; ++i) {
            uint8_t currIdx = matches[i];

            if (i > 0) {
                uint8_t prevIdx = matches[i - 1];

                // Sequential
                if (currIdx == (prevIdx + 1))
                    outScore += sequential_bonus;
            }

            // Check for bonuses based on neighbor character value
            if (currIdx > 0) {
                // Camel case
                char neighbor = strBegin[currIdx - 1];
                char curr = strBegin[currIdx];
                if (::islower(neighbor) && ::isupper(curr))
                    outScore += camel_bonus;

                // Separator
                bool neighborSeparator = neighbor == '_' || neighbor == ' ';
                if (neighborSeparator)
                    outScore += separator_bonus;
            }
            else {
                // First letter
                outScore += first_letter_bonus;
            }
        }
    }

    // Return best result
    if (recursiveMatch && (!matched || bestRecursiveScore > outScore)) {
        // Recursive score is better than "this"
        memcpy(matches, bestRecursiveMatches, maxMatches);
        outScore = bestRecursiveScore;
        return true;
    }
    else if (matched) {
        // "this" score is better than recursive
        return true;
    }
    else {
        // no match
        return false;
    }
}

inline bool fuzzy_match(char const* pattern, char const* str, int& outScore, uint8_t* matches, int maxMatches) {
    int recursionCount = 0;
    int recursionLimit = 10;

    return fuzzy_match_recursive(pattern, str, outScore, str, nullptr, matches, maxMatches, 0, recursionCount, recursionLimit);
}

// Public interface
inline bool fuzzy_match_simple(char const* pattern, char const* str) {
    while (*pattern != '\0' && *str != '\0') {
        if (tolower(*pattern) == tolower(*str))
            ++pattern;
        ++str;
    }

    return *pattern == '\0' ? true : false;
}

inline bool fuzzy_match(char const* pattern, char const* str, int& outScore) {

    uint8_t matches[256];
    return fuzzy_match(pattern, str, outScore, matches, sizeof(matches));
}

inline bool sortbysec_desc(const std::pair<int, int>& a, const std::pair<int, int>& b)
{
    return (b.second < a.second);
}

inline int index_of_key(
    std::vector<std::pair<int, int> > pair_list,
    int key)
{
    for (int i = 0; i < pair_list.size(); ++i)
    {
        auto& p = pair_list[i];
        if (p.first == key)
        {
            return i;
        }
    }
    return -1;
}

namespace ImGui
{

    // Copied from imgui_widgets.cpp
    static float CalcMaxPopupHeightFromItemCount(int items_count)
    {
        ImGuiContext& g = *GImGui;
        if (items_count <= 0)
            return FLT_MAX;
        return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
    }

    static const char* ICON_FA_SEARCH = u8"\ue935";

    inline bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items /*= -1 */)
    {
        ImGuiContext& g = *GImGui;

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        const ImGuiStyle& style = g.Style;

        int items_count = static_cast<int>(items.size());

        // Use imgui Items_ getters to support more input formats.
        const char* preview_value = NULL;
        if (*current_item >= 0 && *current_item < items_count)
            preview_value = items[*current_item].c_str();

        static int focus_idx = -1;
        static char pattern_buffer[256] = { 0 };

        bool value_changed = false;

        const ImGuiID id = window->GetID(label);
        const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id); // copied from BeginCombo
        const bool is_already_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
        const bool is_filtering = is_already_open && pattern_buffer[0] != '\0';

        int show_count = items_count;

        std::vector<std::pair<int, int> > itemScoreVector;
        if (is_filtering)
        {
            // Filter before opening to ensure we show the correct size window.
            // We won't get in here unless the popup is open.
            for (int i = 0; i < items_count; i++)
            {
                int score = 0;
                bool matched = fuzzy_match(pattern_buffer, items[i].c_str(), score);
                if (matched)
                    itemScoreVector.push_back(std::make_pair(i, score));
            }
            std::sort(itemScoreVector.begin(), itemScoreVector.end(), sortbysec_desc);
            int current_score_idx = index_of_key(itemScoreVector, focus_idx);
            if (current_score_idx < 0 && !itemScoreVector.empty())
            {
                focus_idx = itemScoreVector[0].first;
            }
            show_count = static_cast<int>(itemScoreVector.size());
        }

        // Define the height to ensure our size calculation is valid.
        if (popup_max_height_in_items == -1) {
            popup_max_height_in_items = 5;
        }
        popup_max_height_in_items = ImMin(popup_max_height_in_items, show_count);


        if (!(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
        {
            int items = popup_max_height_in_items + 2; // extra for search bar
            SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(items)));
        }

        if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
            return false;


        if (!is_already_open)
        {
            focus_idx = *current_item;
            memset(pattern_buffer, 0, IM_ARRAYSIZE(pattern_buffer));
        }

        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(240, 240, 240, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(0, 0, 0, 255));
        ImGui::PushItemWidth(-FLT_MIN);
        // Filter input
        if (!is_already_open)
            ImGui::SetKeyboardFocusHere();
        InputText("##ComboWithFilter_inputText", pattern_buffer, 256, ImGuiInputTextFlags_AutoSelectAll);

        // Search Icon, you can use it if you load IconsFontAwesome5 https://github.com/juliettef/IconFontCppHeaders
        //const ImVec2 label_size = CalcTextSize(ICON_FA_SEARCH, NULL, true);
        //const ImVec2 search_icon_pos(
        //    ImGui::GetItemRectMax().x - label_size.x - style.ItemInnerSpacing.x * 2,
        //    window->DC.CursorPos.y + style.FramePadding.y + g.FontSize * 0.3f);
        //RenderText(search_icon_pos, ICON_FA_SEARCH);

        ImGui::PopStyleColor(2);

        int move_delta = 0;
        if (IsKeyPressed(ImGuiKey_UpArrow))
        {
            --move_delta;
        }
        else if (IsKeyPressed(ImGuiKey_DownArrow))
        {
            ++move_delta;
        }
        else if (IsKeyPressed(ImGuiKey_PageUp))
        {
            move_delta -= popup_max_height_in_items;
        }
        else if (IsKeyPressed(ImGuiKey_PageDown))
        {
            move_delta += popup_max_height_in_items;
        }

        if (move_delta != 0)
        {
            if (is_filtering)
            {
                int current_score_idx = index_of_key(itemScoreVector, focus_idx);
                if (current_score_idx >= 0)
                {
                    const int count = static_cast<int>(itemScoreVector.size());
                    current_score_idx = ImClamp(current_score_idx + move_delta, 0, count - 1);
                    focus_idx = itemScoreVector[current_score_idx].first;
                }
            }
            else
            {
                focus_idx = ImClamp(focus_idx + move_delta, 0, items_count - 1);
            }
        }

        // Copied from ListBoxHeader
        // If popup_max_height_in_items == -1, default height is maximum 7.
        float height_in_items_f = (popup_max_height_in_items < 0 ? ImMin(items_count, 7) : popup_max_height_in_items) + 0.25f;
        ImVec2 size;
        size.x = 0.0f;
        size.y = GetTextLineHeightWithSpacing() * height_in_items_f + g.Style.FramePadding.y * 2.0f;

        if (ImGui::BeginListBox("##ComboWithFilter_itemList", size))
        {
            for (int i = 0; i < show_count; i++)
            {
                int idx = is_filtering ? itemScoreVector[i].first : i;
                PushID((void*)(intptr_t)idx);
                const bool item_selected = (idx == focus_idx);
                const char* item_text = items[idx].c_str();
                if (Selectable(item_text, item_selected))
                {
                    value_changed = true;
                    *current_item = idx;
                    CloseCurrentPopup();
                }

                if (item_selected)
                {
                    SetItemDefaultFocus();
                    // SetItemDefaultFocus doesn't work so also check IsWindowAppearing.
                    if (move_delta != 0 || IsWindowAppearing())
                    {
                        SetScrollHereY();
                    }
                }
                PopID();
            }
            ImGui::EndListBox();

            if (IsKeyPressed(ImGuiKey_Enter))
            {
                value_changed = true;
                *current_item = focus_idx;
                CloseCurrentPopup();
            }
            else if (IsKeyPressed(ImGuiKey_Escape))
            {
                value_changed = false;
                CloseCurrentPopup();
            }
        }
        ImGui::PopItemWidth();
        ImGui::EndCombo();


        if (value_changed)
            MarkItemEdited(g.LastItemData.ID);

        return value_changed;
    }

} // namespace ImGui
