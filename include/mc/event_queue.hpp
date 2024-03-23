#pragma once

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

class EventQueue
{
public:
    EventQueue() = default;

    EventQueue(EventQueue const&)                    = delete;
    EventQueue(EventQueue&&)                         = delete;
    auto operator=(EventQueue const&) -> EventQueue& = delete;
    auto operator=(EventQueue&&) -> EventQueue&      = delete;

    ~EventQueue() = default;

    template<typename EventType>
    void addListener(std::function<void(EventType const&)> listener)

    {
        m_eventListeners.insert({ typeid(EventType).hash_code(),
                                  [listener = std::move(listener)](Event const& event)
                                  {
                                      listener(*static_cast<const EventType*>(&event));
                                  } });
    }

    template<typename EventType, typename Class>
    void addListener(Class* instance, void (Class::*func)(EventType const&))
    {
        m_eventListeners.insert({ typeid(EventType).hash_code(),
                                  [func, instance](Event const& event)
                                  {
                                      (instance->*func)(*static_cast<const EventType*>(&event));
                                  } });
    }

    template<typename Class, typename... EventTypes>
    void addListeners(Class* instance, void (Class::*... funcs)(EventTypes const&))
    {
        (addListener(instance, funcs), ...);
    }

    template<typename... EventTypes>
    void addListeners(void(... funcs)(EventTypes const&))
    {
        (addListener(funcs), ...);
    }

    template<typename EventType>
    void dispatchEvent(EventType&& event)
    {
        for (auto range { m_eventListeners.equal_range(typeid(event).hash_code()) }; range.first != range.second;
             ++range.first)
        {
            range.first->second(event);
        }
    }

private:
    std::unordered_multimap<std::size_t, std::function<void(Event const&)>> m_eventListeners;
};
