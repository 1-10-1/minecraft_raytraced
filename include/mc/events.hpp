#pragma once

#include "input_manager.hpp"
#include "key.hpp"
#include "mouse_buttons.hpp"
#include "timer.hpp"

#include <glm/vec2.hpp>

enum class BaseEventType : uint8_t
{
    Input,
    Window,
    App
};

enum class EventType : uint8_t
{
    KeyPress,
    KeyHold,
    KeyRelease,
    CursorMove,
    MouseButton,
    MouseScroll,

    WindowClose,
    WindowFocusChanged,
    WindowResize,
    WindowMove,
    WindowFramebufferResize,
    WindowRefresh,
    WindowMinOrMaximize,
    WindowCursorFocusChanged,
    WindowDragAndDrop,

    AppUpdate,
    AppRender,

    EVENT_TYPE_MAX
};

template<typename EventClass>
concept EventSpec = requires() {
    {
        EventClass::eventType
    } -> std::same_as<EventType const&>;
    {
        EventClass::baseEventType
    } -> std::same_as<BaseEventType const&>;
};

class InputEvent
{
public:
    [[nodiscard]] auto getInputManager() const -> InputManager const* { return m_inputManager; }

    constexpr static auto baseEventType = BaseEventType::Input;

protected:
    explicit InputEvent(InputManager const* inputManager) : m_inputManager(inputManager) {};

private:
    InputManager const* m_inputManager;
};

class WindowEvent
{
public:
    constexpr static auto baseEventType = BaseEventType::Window;

protected:
    WindowEvent() = default;
};

class AppEvent
{
public:
    constexpr static auto baseEventType = BaseEventType::App;

protected:
    AppEvent() = default;
};

class KeyPressEvent : public InputEvent
{
public:
    explicit KeyPressEvent(InputManager const* inputManager, Key pressedKey, int mods, bool repeat)
        : InputEvent { inputManager }, key { pressedKey }, modifiers { mods }, repeated { repeat }
    {
    }

    Key key;
    int modifiers;
    bool repeated;

    constexpr static auto eventType = EventType::KeyPress;
};

class KeyReleaseEvent : public InputEvent
{
public:
    explicit KeyReleaseEvent(InputManager const* inputManager, Key releasedKey, int mods)
        : InputEvent { inputManager }, key { releasedKey }, modifiers { mods }
    {
    }

    Key key;
    int modifiers;

    constexpr static auto eventType = EventType::KeyRelease;
};

class KeyHoldEvent : public InputEvent
{
public:
    explicit KeyHoldEvent(InputManager const* inputManager, Key heldDownKey)
        : InputEvent { inputManager }, key { heldDownKey }
    {
    }

    Key key;

    constexpr static auto eventType = EventType::KeyHold;
};

class CursorMoveEvent : public InputEvent
{
public:
    explicit CursorMoveEvent(InputManager const* inputManager, glm::uvec2 pos)
        : InputEvent { inputManager }, position { pos }
    {
    }

    glm::ivec2 position;

    constexpr static auto eventType = EventType::CursorMove;
};

class MouseButtonEvent : public InputEvent
{
public:
    enum class Action
    {
        Pressed  = GLFW_PRESS,
        Released = GLFW_RELEASE
    };

    MouseButtonEvent(InputManager const* inputManager, MouseButton mouseButton, Action buttonAction, int mods)
        : InputEvent { inputManager },
          button { mouseButton },
          action { buttonAction },
          modifiers { mods },
          position { inputManager->getCurrentCursorPosition() }
    {
    }

    MouseButton button;
    Action action;
    int modifiers;
    glm::uvec2 position;

    constexpr static auto eventType = EventType::MouseButton;
};

class MouseScrollEvent : public InputEvent
{
public:
    MouseScrollEvent(InputManager const* inputManager, glm::vec2 delta)
        : InputEvent { inputManager }, wheelDelta { delta }, position { getInputManager()->getCurrentCursorPosition() }
    {
    }

    glm::vec2 wheelDelta;
    glm::uvec2 position;

    constexpr static auto eventType = EventType::MouseScroll;
};

class WindowResizeEvent : public WindowEvent
{
public:
    explicit WindowResizeEvent(glm::uvec2 size) : dimensions { size } {}

    glm::uvec2 dimensions;

    constexpr static auto eventType = EventType::WindowResize;
};

class WindowCloseEvent : public WindowEvent
{
public:
    WindowCloseEvent() = default;

    constexpr static auto eventType = EventType::WindowResize;
};

class WindowRefreshEvent : public WindowEvent
{
public:
    WindowRefreshEvent() = default;

    constexpr static auto eventType = EventType::WindowRefresh;
};

class CursorFocusChangedEvent : public WindowEvent
{
public:
    enum State
    {
        Focused,
        Defocused
    };

    explicit CursorFocusChangedEvent(State focusState) : state { focusState } {}

    State state;

    constexpr static auto eventType = EventType::WindowCursorFocusChanged;
};

class WindowMoveEvent : public WindowEvent
{
public:
    explicit WindowMoveEvent(glm::vec2 pos) : position { pos } {}

    glm::vec2 position;

    constexpr static auto eventType = EventType::WindowMove;
};

class WindowMinOrMaximizeEvent : public WindowEvent
{
public:
    enum State
    {
        Minimized,
        Maximized
    };

    explicit WindowMinOrMaximizeEvent(State windowState) : state { windowState } {}

    State state;

    constexpr static auto eventType = EventType::WindowMinOrMaximize;
};

class WindowFramebufferResizeEvent : public WindowEvent
{
public:
    explicit WindowFramebufferResizeEvent(glm::uvec2 size) : dimensions { size } {}

    glm::uvec2 dimensions;

    constexpr static auto eventType = EventType::WindowFramebufferResize;
};

class WindowDragAndDropEvent : public WindowEvent
{
public:
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    WindowDragAndDropEvent(size_t count, char const* pathsarray[]) : paths(count)
    {
        for (size_t i = 0; i < count; i++)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            paths[i] = pathsarray[i];
        }
    }

    std::vector<std::string_view> paths;

    constexpr static auto eventType = EventType::WindowDragAndDrop;
};

class WindowFocusChangedEvent : public WindowEvent
{
public:
    enum FocusState
    {
        Focused,
        Defocused
    };

    explicit WindowFocusChangedEvent(FocusState focusState) : state { focusState } {}

    FocusState const state;

    constexpr static auto eventType = EventType::WindowFocusChanged;
};

class AppUpdateEvent : public AppEvent
{
public:
    explicit AppUpdateEvent(Timer const& timer) : globalTimer { timer } {}

    constexpr static auto eventType = EventType::AppUpdate;

    Timer const& globalTimer;
};

class AppRenderEvent : public AppEvent
{
public:
    AppRenderEvent() = default;

    constexpr static auto eventType = EventType::AppRender;
};
