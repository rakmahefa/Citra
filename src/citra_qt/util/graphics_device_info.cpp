// Copyright 2023 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra_qt/util/graphics_device_info.h"

#include "common/logging/log.h"
#ifdef ENABLE_VULKAN
#include "video_core/renderer_vulkan/vk_instance.h"
#include <vulkan/vulkan.hpp>
#endif
#ifdef ENABLE_OPENGL
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#endif

#ifdef ENABLE_VULKAN
#include "video_core/renderer_vulkan/vk_instance.h"
#endif

#ifdef ENABLE_OPENGL
QString GetOpenGLRenderer() {
    QOffscreenSurface surface;
    surface.create();

    QOpenGLContext context;
    if (context.create()) {
        context.makeCurrent(&surface);
        return QString::fromUtf8(context.functions()->glGetString(GL_RENDERER));
    } else {
        return QStringLiteral("");
    }
}
#endif

#ifdef ENABLE_VULKAN
std::vector<QString> GetVulkanPhysicalDevices() {
    std::vector<QString> result;
    try {
        Vulkan::Instance instance{};
        const auto physical_devices = instance.GetPhysicalDevices();

        LOG_INFO(Frontend, "Vulkan detection: found {} physical device(s)", physical_devices.size());
        for (const vk::PhysicalDevice physical_device : physical_devices) {
            const vk::PhysicalDeviceProperties props = physical_device.getProperties();
            const QString name = QString::fromUtf8(props.deviceName, -1);
            LOG_INFO(Frontend, "Vulkan device: {} (API {}.{})", name.toStdString(),
                     VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion));
            result.push_back(name);
        }
    } catch (const std::exception& err) {
        LOG_ERROR(Frontend, "Error occurred while querying for Vulkan physical devices: {}", err.what());
    } catch (...) {
        LOG_ERROR(Frontend, "Unknown error occurred while querying for Vulkan physical devices");
    }

    return result;
}
#endif
