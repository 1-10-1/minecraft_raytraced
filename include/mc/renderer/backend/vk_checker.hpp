#pragma once

#include <source_location>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    class ResultGrabber
    {
    public:
        // NOLINTNEXTLINE(google-explicit-constructor)
        ResultGrabber(VkResult res, std::source_location loc = std::source_location::current()) : result { res }, location { loc }
        {
        }

        VkResult result;
        std::source_location location;
    };

    constexpr struct DummyToken
    {
    } vkResultChecker;

    void operator>>(ResultGrabber grabber, DummyToken /*unused*/);
}  // namespace renderer::backend
