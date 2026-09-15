#pragma once

#include <array>

#include "PlatformConfig.h"

struct PopupLayoutInput {
    RECT client{};
    int searchHeight = 23;
    int titleWidth = 0;
    bool searchVisible = false;
    bool showFooter = true;
    bool pinsAtBottom = false;
    int pinCount = 0;
    int historyCount = 0;
};

struct PopupLayoutResult {
    RECT title{};
    RECT search{};
    RECT searchEdit{};
    RECT searchClear{};
    RECT previewToggle{};
    RECT pins{};
    RECT history{};
    std::array<RECT, 4> footer{};
    int pinSeparatorY = -1;
    int footerSeparatorY = -1;
    bool showTitle = false;
    bool showSearchClear = false;
    bool showPreviewToggle = false;
    bool showPins = false;
    bool showHistory = false;
};

class PopupLayout {
public:
    static PopupLayoutResult Calculate(const PopupLayoutInput& input);
};
