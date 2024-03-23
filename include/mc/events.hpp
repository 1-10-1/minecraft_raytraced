#pragma once

#include "input_manager.hpp"
#include "key.hpp"
#include "mouse_buttons.hpp"

#include <bitset>

#include <glm/vec2.hpp>

class Event
{
public:
    enum class BaseType
    {
        Input,
        Window,
        App
    };

    [[nodiscard]] auto getBaseType() const -> BaseType { return m_baseType; }

protected:
    explicit Event(BaseType baseType) : m_baseType(baseType) {}

private:
    BaseType m_baseType;
};

class InputEvent : public Event
{
public:
    enum class Type
    {
        KeyPress,
        KeyHold,
        KeyRelease,
        CursorMove,
        MouseButton,
        MouseScroll
    };

    [[nodiscard]] auto getInputManager() const -> InputManager const* { return m_inputManager; }

    [[nodiscard]] auto getInputType() const -> Type { return m_eventType; }

protected:
    InputEvent(Type inputType, InputManager const* inputManager)
        : Event(Event::BaseType::Input), m_eventType(inputType), m_inputManager(inputManager) {};

private:
    Type m_eventType;
    InputManager const* m_inputManager;
};

class WindowEvent : public Event
{
public:
    enum class Type
    {
        WindowClose,
        WindowFocus,
        WindowResize,
        WindowMove,
        WindowFramebufferResize,
        WindowRefresh,
        WindowMinOrMaximize,
        CursorFocus,
        DragAndDrop
    };

    explicit WindowEvent(Type eventType) : Event(Event::BaseType::Window), m_eventType(eventType) {};

    [[nodiscard]] auto getEventType() const -> Type { return m_eventType; }

private:
    Type m_eventType;
};

class AppEvent : public Event
{
public:
    enum class Type
    {
        Update,
        Render
    };

    explicit AppEvent(Type eventType) : Event(Event::BaseType::App), m_eventType(eventType) {};

    [[nodiscard]] auto getEventType() const -> Type { return m_eventType; }

private:
    Type m_eventType;
};

class KeyPressEvent : public InputEvent
{
public:
    explicit KeyPressEvent(InputManager const* inputManager, Key pressedKey, int mods, bool repeat)
        : InputEvent { Type::KeyPress, inputManager }, key { pressedKey }, modifiers { mods }, repeated { repeat }
    {
    }

    Key key;
    int modifiers;
    bool repeated;
};

class KeyReleaseEvent : public InputEvent
{
public:
    explicit KeyReleaseEvent(InputManager const* inputManager, Key releasedKey, int mods)
        : InputEvent { Type::KeyRelease, inputManager }, key { releasedKey }, modifiers { mods }
    {
    }

    Key key;
    int modifiers;
};

class KeyHoldEvent : public InputEvent
{
public:
    explicit KeyHoldEvent(InputManager const* inputManager, Key heldDownKey)
        : InputEvent { Type::KeyHold, inputManager }, key { heldDownKey }
    {
    }

    Key key;
};

class CursorMoveEvent : public InputEvent
{
public:
    explicit CursorMoveEvent(InputManager const* inputManager, glm::uvec2 pos)
        : InputEvent { Type::CursorMove, inputManager }, position { pos }
    {
    }

    glm::ivec2 position;
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
        : InputEvent { Type::MouseButton, inputManager },
          button { mouseButton },
          action { buttonAction },
          modifiers { mods },
          position { getInputManager()->getCurrentCursorPosition() }
    {
    }

    MouseButton button;
    Action action;
    int modifiers;
    glm::uvec2 position;
};

class MouseScrollEvent : public InputEvent
{
public:
    MouseScrollEvent(InputManager const* inputManager, glm::vec2 delta)
        : InputEvent { Type::MouseScroll, inputManager },
          wheelDelta { delta },
          position { getInputManager()->getCurrentCursorPosition() }
    {
    }

    glm::vec2 wheelDelta;
    glm::uvec2 position;
};

class WindowResizeEvent : public WindowEvent
{
public:
    explicit WindowResizeEvent(glm::uvec2 size) : WindowEvent { Type::WindowResize }, dimensions { size } {}

    glm::uvec2 dimensions;
};

class WindowCloseEvent : public WindowEvent
{
public:
    explicit WindowCloseEvent() : WindowEvent { Type::WindowClose } {}
};

class WindowRefreshEvent : public WindowEvent
{
public:
    explicit WindowRefreshEvent() : WindowEvent { Type::WindowRefresh } {}
};

class WindowFocusEvent : public WindowEvent
{
public:
    enum State
    {
        Focused,
        Defocused
    };

    explicit WindowFocusEvent(State focusState) : WindowEvent { Type::WindowFocus }, state { focusState } {}

    State state;
};

class WindowMoveEvent : public WindowEvent
{
public:
    explicit WindowMoveEvent(glm::vec2 pos) : WindowEvent { Type::WindowMove }, position { pos } {}

    glm::vec2 position;
};

class WindowMinOrMaximizeEvent : public WindowEvent
{
public:
    enum State
    {
        Minimized,
        Maximized
    };

    explicit WindowMinOrMaximizeEvent(State windowState)
        : WindowEvent { Type::WindowMinOrMaximize }, state { windowState }
    {
    }

    State state;
};

class WindowFramebufferResizeEvent : public WindowEvent
{
public:
    explicit WindowFramebufferResizeEvent(glm::uvec2 size)
        : WindowEvent { Type::WindowFramebufferResize }, dimensions { size }
    {
    }

    glm::uvec2 dimensions;
};

class WindowDragAndDropEvent : public WindowEvent
{
public:
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    WindowDragAndDropEvent(size_t count, char const* pathsarray[]) : WindowEvent { Type::DragAndDrop }, paths(count)
    {
        for (size_t i = 0; i < count; i++)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            paths[i] = pathsarray[i];
        }
    }

    std::vector<std::string_view> paths;
};

class CursorFocusEvent : public WindowEvent
{
public:
    enum FocusState
    {
        Focused,
        Defocused
    };

    explicit CursorFocusEvent(FocusState focusState) : WindowEvent { Type::CursorFocus }, state { focusState } {}

    FocusState state;
};

class AppUpdateEvent : public AppEvent
{
public:
    AppUpdateEvent() : AppEvent { Type::Update } {};
};

class AppRenderEvent : public AppEvent
{
public:
    AppRenderEvent() : AppEvent { Type::Render } {};
};
