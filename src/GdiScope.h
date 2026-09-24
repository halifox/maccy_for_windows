#pragma once

#include "PlatformConfig.h"

class ScopedDcState {
public:
    explicit ScopedDcState(HDC dc) noexcept
        : m_dc(dc), m_savedState(dc != nullptr ? ::SaveDC(dc) : 0) {}

    ~ScopedDcState() {
        Restore();
    }

    ScopedDcState(const ScopedDcState &) = delete;
    ScopedDcState &operator=(const ScopedDcState &) = delete;

    void Restore() noexcept {
        if (m_dc != nullptr && m_savedState > 0) {
            ::RestoreDC(m_dc, m_savedState);
            m_savedState = 0;
        }
    }

private:
    HDC m_dc = nullptr;
    int m_savedState = 0;
};

class ScopedGdiObjectSelection {
public:
    ScopedGdiObjectSelection(HDC dc, HGDIOBJ object) noexcept
        : m_dc(dc),
          m_previous(dc != nullptr && object != nullptr ? ::SelectObject(dc, object) : nullptr),
          m_selected(m_previous != nullptr && m_previous != HGDI_ERROR) {}

    ~ScopedGdiObjectSelection() {
        if (m_selected) {
            ::SelectObject(m_dc, m_previous);
        }
    }

    ScopedGdiObjectSelection(const ScopedGdiObjectSelection &) = delete;
    ScopedGdiObjectSelection &operator=(const ScopedGdiObjectSelection &) = delete;

    bool IsSelected() const noexcept { return m_selected; }

private:
    HDC m_dc = nullptr;
    HGDIOBJ m_previous = nullptr;
    bool m_selected = false;
};
