#pragma once

#include <bitset>
#include <unordered_set>

#include "key.hpp"
#include "mouse_buttons.hpp"

#include <glm/ext/vector_uint2.hpp>
#include <magic_enum.hpp>

class Window;

class InputManager
{
    friend class Window;

public:
    [[nodiscard]] auto isDown(Key key) const -> bool
    {
        return m_keyStates.test(static_cast<size_t>(key));
    }

    [[nodiscard]] auto isDown(MouseButton button) const -> bool
    {
        return m_keyStates.test(static_cast<size_t>(button));
    }

    [[nodiscard]] auto getDownKeys() const -> std::unordered_set<Key> const& { return m_downKeys; }

    [[nodiscard]] auto getCurrentCursorPosition() const -> glm::uvec2 const&
    {
        return m_currentCursorPos;
    }

private:
    InputManager() { m_downKeys.reserve(kNumKeys); }

    void setKey(Key key, bool enable = true)
    {
        m_keyStates.set(magic_enum::enum_underlying(key), enable);

        if (enable)
        {
            m_downKeys.insert(key);
        }
        else
        {
            m_downKeys.erase(key);
        }
    }

    void setButton(MouseButton button, bool enable = true)
    {
        m_keyStates.set(static_cast<size_t>(button), enable);

        if (enable)
        {
            m_downButtons.insert(button);
        }
        else
        {
            m_downButtons.erase(button);
        }
    }

    void setCursorPos(glm::uvec2 const& pos) { m_currentCursorPos = pos; }

    std::bitset<32> m_buttonStates {};
    std::bitset<384> m_keyStates {};

    std::unordered_set<MouseButton> m_downButtons {};
    std::unordered_set<Key> m_downKeys {};

    glm::uvec2 m_currentCursorPos { 0, 0 };
};
