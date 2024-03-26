#pragma once

#include <any>
#include <concepts>
#include <functional>
#include <glm/ext/vector_float2.hpp>

#include "events.hpp"
#include "input_manager.hpp"
#include "key.hpp"
#include "utils.hpp"

// Whenever you have time, please work on fixing the following issue:
// When an event listener is registered with a "this" parameter, make sure that the object
// that "this" points to does not get destroyed without notifying the event queue so that it can be
// properly cleaned up.

class EventManager
{
public:
    EventManager() = default;

    EventManager(EventManager const&)                    = delete;
    EventManager(EventManager&&)                         = delete;
    auto operator=(EventManager const&) -> EventManager& = delete;
    auto operator=(EventManager&&) -> EventManager&      = delete;

    ~EventManager() = default;

    template<EventSpec Event>
    void addListener(std::function<void(Event const&)> listener)
    {
        m_eventListeners.insert({ Event::eventType,
                                  [listener = std::move(listener)](std::any const& event)
                                  {
                                      listener(std::any_cast<Event const&>(event));
                                  } });
    }

    template<EventSpec Event, typename Class>
    void addListener(Class* instance, void (Class::*func)(Event const&))
    {
        m_eventListeners.insert({ Event::eventType,
                                  [func, instance](Event const& event)
                                  {
                                      (instance->*func)(std::any_cast<const EventType&>(event));
                                  } });
    }

    template<typename Class, EventSpec... Events>
    void addListeners(Class* instance, void (Class::*... funcs)(Events const&))
    {
        (addListener(instance, funcs), ...);
    }

    template<EventSpec... Events>
    void addListeners(void(... funcs)(Events const&))
    {
        (addListener(funcs), ...);
    }

    template<EventSpec Event>
    void dispatchEvent(Event const& event)
    {
        for (auto range { m_eventListeners.equal_range(Event::eventType) };
             range.first != range.second;
             ++range.first)
        {
            range.first->second(event);
        }
    }

private:
    std::unordered_multimap<EventType, std::function<void(std::any)>> m_eventListeners;
};
