#include "mc/defines.hpp"
#include "mc/exceptions.hpp"
#include "mc/renderer/backend/vk_checker.hpp"
#include "mc/renderer/backend/vk_result_messages.hpp"

namespace renderer::backend
{
    void operator>>([[maybe_unused]] ResultGrabber grabber, DummyToken /*unused*/)
    {
        if constexpr (!kDebug)
        {
            return;
        }

        if (grabber.result != VK_SUCCESS)
        {
            MC_THROW Error(GraphicsError, vkResultToStr(grabber.result).data(), grabber.location);
        }
    }
}  // namespace renderer::backend
