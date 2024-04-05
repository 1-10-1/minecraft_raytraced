#pragma once

#include "events.hpp"
#include "mc/defines.hpp"

#include <any>
#include <functional>
#include <utility>

#include <glm/ext/vector_float2.hpp>
#include <magic_enum.hpp>
#include <tracy/Tracy.hpp>

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
        m_eventListeners[std::to_underlying(Event::eventType)].push_back(
            [listener = std::move(listener)](std::any const& event)
            {
                listener(std::any_cast<Event const&>(event));
            });
    }

    template<EventSpec Event>
    void addListener(void (*listener)(Event const&))
    {
        addListener(std::function(listener));
    }

    template<EventSpec Event, typename Class>
    void addListener(Class* instance, void (Class::*func)(Event const&))
    {
        m_eventListeners[std::to_underlying(Event::eventType)].push_back(
            [func, instance](std::any const& event)
            {
                (instance->*func)(std::any_cast<Event const&>(event));
            });
    };

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
        ZoneScopedN("Event Dispatch");

        [[maybe_unused]] std::string_view eventName = magic_enum::enum_name(Event::eventType);

        ZoneText(eventName.data(), eventName.size());

        auto event_any = std::make_any<Event const&>(event);

        for (auto const& listener : m_eventListeners[static_cast<size_t>(Event::eventType)])
        {
            listener(event_any);
        }
    }

private:
    std::array<std::vector<std::function<void(std::any const&)>>, static_cast<size_t>(EventType::EVENT_TYPE_MAX)>
        m_eventListeners {};
};
