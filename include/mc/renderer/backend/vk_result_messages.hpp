#pragma once

#include <string_view>
#include <unordered_map>

#include <vulkan/vulkan_core.h>

namespace renderer::backend
{
    inline auto vkResultToStr(VkResult result) -> std::string_view const&
    {
        // clang-format off
    static std::unordered_map<VkResult, std::string_view> vkResultNames {
        { VK_SUCCESS,                                    "Success"                         },
        { VK_NOT_READY,                                  "Not ready"                       },
        { VK_TIMEOUT,                                    "Timeout"                         },
        { VK_EVENT_SET,                                  "Event set"                       },
        { VK_EVENT_RESET,                                "Event reset"                     },
        { VK_INCOMPLETE,                                 "Incomplete"                      },
        { VK_ERROR_OUT_OF_HOST_MEMORY,                   "Out of host memory"              },
        { VK_ERROR_OUT_OF_DEVICE_MEMORY,                 "Out of device memory"            },
        { VK_ERROR_INITIALIZATION_FAILED,                "Initialization failed"           },
        { VK_ERROR_DEVICE_LOST,                          "Device lost"                     },
        { VK_ERROR_MEMORY_MAP_FAILED,                    "Memory map failed"               },
        { VK_ERROR_LAYER_NOT_PRESENT,                    "Layer not present"               },
        { VK_ERROR_EXTENSION_NOT_PRESENT,                "Extension not present"           },
        { VK_ERROR_FEATURE_NOT_PRESENT,                  "Feature not present"             },
        { VK_ERROR_INCOMPATIBLE_DRIVER,                  "Incompatible driver"             },
        { VK_ERROR_TOO_MANY_OBJECTS,                     "Too many objects"                },
        { VK_ERROR_FORMAT_NOT_SUPPORTED,                 "Format not supported"            },
        { VK_ERROR_FRAGMENTED_POOL,                      "Fragmented pool"                 },
        { VK_ERROR_UNKNOWN,                              "Unknown"                         },
        { VK_ERROR_OUT_OF_POOL_MEMORY,                   "Out of pool memory"              },
        { VK_ERROR_INVALID_EXTERNAL_HANDLE,              "Invalid external handle"         },
        { VK_ERROR_FRAGMENTATION,                        "Fragmentation"                   },
        { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,       "Invalid opaque capture address"  },
        { VK_PIPELINE_COMPILE_REQUIRED,                  "Pipeline compile required"       },
        { VK_ERROR_SURFACE_LOST_KHR,                     "Surface lost"                    },
        { VK_ERROR_NATIVE_WINDOW_IN_USE_KHR,             "Native window in use"            },
        { VK_SUBOPTIMAL_KHR,                             "Suboptimal"                      },
        { VK_ERROR_OUT_OF_DATE_KHR,                      "Out of date"                     },
        { VK_ERROR_INCOMPATIBLE_DISPLAY_KHR,             "Incompatible display"            },
        { VK_ERROR_VALIDATION_FAILED_EXT,                "Validation failed"               },
        { VK_ERROR_INVALID_SHADER_NV,                    "Invalid shader nv"               },
        { VK_ERROR_NOT_PERMITTED_KHR,                    "Not permitted"                   },
        { VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT,  "Full screen exclusive mode lost" },
        { VK_THREAD_IDLE_KHR,                            "Thread idle"                     },
        { VK_THREAD_DONE_KHR,                            "Thread done"                     },
        { VK_OPERATION_DEFERRED_KHR,                     "Operation deferred"              },
        { VK_OPERATION_NOT_DEFERRED_KHR,                 "Operation not deferred"          },
        { VK_ERROR_COMPRESSION_EXHAUSTED_EXT,            "Compression exhausted"           },
        { VK_ERROR_OUT_OF_POOL_MEMORY_KHR,               "Out of pool memory"              },
        { VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR,          "Invalid external handle"         },
        { VK_ERROR_FRAGMENTATION_EXT,                    "Fragmentation"                   },
        { VK_ERROR_NOT_PERMITTED_EXT,                    "Not permitted"                   },
        { VK_ERROR_INVALID_DEVICE_ADDRESS_EXT,           "Invalid device address"          },
        { VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR,   "Invalid opaque capture address"  },
        { VK_PIPELINE_COMPILE_REQUIRED_EXT,              "Pipeline compile required"       },
        { VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT,        "Pipeline compile required"       },
        { VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT,
          "Invalid drm format modifier plane layout"                                       },
    };
        // clang-format on

        return vkResultNames.at(result);
    }
}  // namespace renderer::backend
