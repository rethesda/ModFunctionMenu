#include <PCH.h>

#include <spdlog/sinks/basic_file_sink.h>

#include "Configuration.h"
#include "EventSink.h"
#include "Util/Win.h"

namespace
{
    void InitLogger()
    {
        auto path = SKSE::log::log_directory();
        if (!path) {
            SKSE::stl::report_and_fail("Failed to find SKSE logging directory."sv);
        }
        *path /= SKSE::PluginDeclaration::GetSingleton()->GetName();
        *path += L".log"sv;

        auto logger = spdlog::basic_logger_mt("Global", PathToStr(*path), true);
#ifndef _DEBUG
        logger->set_level(spdlog::level::info);
        logger->flush_on(spdlog::level::info);
#else
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
#endif

        spdlog::set_default_logger(std::move(logger));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
    }

    void OnMessage(SKSE::MessagingInterface::Message* a_message)
    {
        switch (a_message->type) {
        case SKSE::MessagingInterface::kInputLoaded:
            InputEventSink::Register();
            break;
        default:
            break;
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    InitLogger();

    if (auto os_ver = GetOsVersion()) {
        SKSE::log::info("OS Version: {}", os_ver->string("."sv));
    } else {
        SKSE::log::info("OS Version: Unknown");
    }
    SKSE::log::info("Game Version: {}", a_skse->RuntimeVersion().string("."sv));
#ifdef _DEBUG
    SKSE::log::debug("Base Address: 0x{:016X}", REL::Module::get().base());
#endif

    auto plugin = SKSE::PluginDeclaration::GetSingleton();
    SKSE::log::info("{} {} is loading...", plugin->GetName(), plugin->GetVersion().string("."sv));

    SKSE::Init(a_skse);

    Configuration::Init();

    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

    SKSE::log::info("{} has finished loading.", plugin->GetName());
    return true;
}
