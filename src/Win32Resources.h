#pragma once

#include "PlatformConfig.h"

#include <utility>

template <typename Handle, auto Destroy>
class UniqueWin32Handle {
public:
    UniqueWin32Handle() noexcept = default;
    explicit UniqueWin32Handle(Handle handle) noexcept : m_handle(handle) {}

    ~UniqueWin32Handle() { Reset(); }

    UniqueWin32Handle(const UniqueWin32Handle &) = delete;
    UniqueWin32Handle &operator=(const UniqueWin32Handle &) = delete;

    UniqueWin32Handle(UniqueWin32Handle &&other) noexcept
        : m_handle(other.Release()) {}

    UniqueWin32Handle &operator=(UniqueWin32Handle &&other) noexcept {
        if (this != &other) {
            Reset(other.Release());
        }
        return *this;
    }

    Handle Get() const noexcept { return m_handle; }
    explicit operator bool() const noexcept { return m_handle != nullptr; }

    Handle Release() noexcept {
        return std::exchange(m_handle, nullptr);
    }

    void Reset(Handle replacement = nullptr) noexcept {
        if (replacement == m_handle) {
            return;
        }
        if (m_handle != nullptr) {
            (void)Destroy(m_handle);
        }
        m_handle = replacement;
    }

private:
    Handle m_handle = nullptr;
};

using UniqueIcon = UniqueWin32Handle<HICON, ::DestroyIcon>;
using UniqueMenu = UniqueWin32Handle<HMENU, ::DestroyMenu>;
using UniqueGlobal = UniqueWin32Handle<HGLOBAL, ::GlobalFree>;

class ScopedGlobalLock {
public:
    explicit ScopedGlobalLock(HGLOBAL handle) noexcept
        : m_handle(handle), m_data(handle != nullptr ? ::GlobalLock(handle) : nullptr) {}

    ~ScopedGlobalLock() { Unlock(); }

    ScopedGlobalLock(const ScopedGlobalLock &) = delete;
    ScopedGlobalLock &operator=(const ScopedGlobalLock &) = delete;

    void *Data() const noexcept { return m_data; }

    void Unlock() noexcept {
        if (m_data != nullptr) {
            ::GlobalUnlock(m_handle);
            m_data = nullptr;
        }
    }

private:
    HGLOBAL m_handle = nullptr;
    void *m_data = nullptr;
};
