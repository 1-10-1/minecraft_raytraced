#pragma once

#include "events.hpp"
#include "logger.hpp"

#include <any>
#include <ranges>
#include <utility>

#include <glm/ext/vector_float2.hpp>
#include <magic_enum.hpp>
#include <tracy/Tracy.hpp>

namespace rn = std::ranges;
namespace vi = std::ranges::views;

constexpr void dormantEventCallback(std::any const& /*unused*/) {}

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
    void subscribe(std::function<void(Event const&)> listener)
    {
        m_eventListeners[std::to_underlying(Event::eventType)].push_back(std::move(listener));
    }

    template<EventSpec Event, typename Class>
    void subscribe(Class* instance, std::functionvoid (Class::*func)(Event const&))
    {
        m_eventListeners[std::to_underlying(Event::eventType)].push_back(std::function<void(Event const&)>(func));
    };

    template<typename Class, EventSpec... Events>
    void subscribe(Class* instance, void (Class::*... funcs)(Events const&))

    {
        (subscribe(instance, funcs), ...);
    }

    template<EventSpec... Events>
    void subscribe(void(... funcs)(Events const&))
    {
        (subscribe(funcs), ...);
    }

    template<typename Class, EventSpec Event>
    void unsubscribe(Class* instance, void (Class::*func)(Event const&))
    {
        for (auto const& [index, callback] : vi::enumerate(m_eventListeners[static_cast<size_t>(Event::eventType)]))
        {
            if (callback.type().hash_code() != typeid(func).hash_code())
            {
                return;
            }

            auto callbackPtr = std::any_cast<void (Class::*)(Event const&)>(callback);

            if (callbackPtr == func)
            {
                logger::info("Found the callback ptr for event {} at position {}",
                             magic_enum::enum_name(Event::eventType).data(),
                             index + 1);
                return;
            }
        }

        logger::info("Could not locate a single callback for event {}. Size: {}",
                     magic_enum::enum_name(Event::eventType).data(),
                     m_eventListeners[static_cast<size_t>(Event::eventType)].size());
    };

    template<EventSpec Event>
    void dispatchEvent(Event const& event)
    {
        ZoneScopedN("Event Dispatch");

        [[maybe_unused]] std::string_view eventName = magic_enum::enum_name(Event::eventType);

        ZoneText(eventName.data(), eventName.size());

        logger::debug("Hi {}", magic_enum::enum_name(Event::eventType));

        for (std::any const& listener : m_eventListeners[static_cast<size_t>(Event::eventType)])
        {
            std::any_cast<std::function<void(Event const&)>>(listener)(event);
        }

        logger::debug("bye");
    }

private:
    std::array<std::vector<std::any>, static_cast<size_t>(EventType::EVENT_TYPE_MAX)> m_eventListeners {};

    // create a dormant array for each event type and store its index
};
