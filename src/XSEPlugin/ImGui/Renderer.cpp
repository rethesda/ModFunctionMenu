#include "Renderer.h"

#include <dxgi.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>

#include <XSEPlugin/Base/Configuration.h>
#include <XSEPlugin/Base/Translation.h>
#include <XSEPlugin/ImGui/Input.h>
#include <XSEPlugin/ImGui/Menu.h>
#include <XSEPlugin/InputManager.h>
#include <XSEPlugin/Util/CLib/Hook.h>

namespace ImGui
{
    struct WndProcHook
    {
        static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            if (uMsg == WM_KILLFOCUS) {
                ClearInputEvent();
            }
            return func(hWnd, uMsg, wParam, lParam);
        }

        static inline WNDPROC func;
    };

    struct D3DInitHook : CLib::CallHook<D3DInitHook>
    {
        static void thunk()
        {
            func();
            Renderer::GetSingleton()->Init();
        }

        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr std::string_view name{ "D3DInit"sv };

        static constexpr REL::VariantID     id{ 75595, 77226, 0xDC5530 };
        static constexpr REL::VariantOffset offset{ 0x9, 0x275, 0x9 };
    };

    struct DXGIPresentHook : CLib::CallHook<DXGIPresentHook>
    {
        static void thunk(std::uint32_t a_timer)
        {
            func(a_timer);
            Renderer::GetSingleton()->Run();
        }

        static inline REL::Relocation<decltype(thunk)> func;

        static constexpr std::string_view name{ "DXGIPresent"sv };

        static constexpr REL::VariantID     id{ 75461, 77246, 0xDBBDD0 };
        static constexpr REL::VariantOffset offset{ 0x9, 0x9, 0x15 };
    };

    void Renderer::Install()
    {
        D3DInitHook::Install();
        DXGIPresentHook::Install();
    }

    void Renderer::Init()
    {
        SKSE::log::info("Initializing ImGui...");

        const auto renderer = RE::BSGraphics::Renderer::GetSingleton();
        if (!renderer) {
            SKSE::log::error("couldn't find renderer");
            return;
        }

        const auto swapChain = (IDXGISwapChain*)renderer->data.renderWindows[0].swapChain;
        if (!swapChain) {
            SKSE::log::error("couldn't find swapChain");
            return;
        }

        DXGI_SWAP_CHAIN_DESC desc{};
        if (FAILED(swapChain->GetDesc(std::addressof(desc)))) {
            SKSE::log::error("IDXGISwapChain::GetDesc failed.");
            return;
        }

        const auto device = (ID3D11Device*)renderer->data.forwarder;
        const auto context = (ID3D11DeviceContext*)renderer->data.context;

        ImGui::CreateContext();

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.IniFilename = nullptr;
        io.MouseDrawCursor = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        if (!ImGui_ImplWin32_Init(desc.OutputWindow)) {
            SKSE::log::error("ImGui initialization failed (Win32)");
            return;
        }

        if (!ImGui_ImplDX11_Init(device, context)) {
            SKSE::log::error("ImGui initialization failed (DX11)");
            return;
        }

        WndProcHook::func = reinterpret_cast<WNDPROC>(
            SetWindowLongPtrA(desc.OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook::thunk)));
        if (!WndProcHook::func) {
            SKSE::log::warn("SetWindowLongPtrA failed!");
        }

        Load();

        _isInit.store(true);
        SKSE::log::info("ImGui initialized.");
    }

    void Renderer::Run()
    {
        if (!IsInit() || !IsEnable()) {
            return;
        }

        if (Configuration::IsVersionChanged(_configVersion) || Translation::IsVersionChanged(_transVersion)) {
            Load();
        } else {
            fonts.Refresh();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        {
            // trick imgui into rendering at game's real resolution (ie. if upscaled with Display Tweaks)
            static const auto screenSize = RE::BSGraphics::Renderer::GetScreenSize();

            auto& io = ImGui::GetIO();
            io.DisplaySize.x = static_cast<float>(screenSize.width);
            io.DisplaySize.y = static_cast<float>(screenSize.height);
        }
        ImGui::NewFrame();
        {
            if (auto menu = Menu::GetSingleton(); menu->IsOpen()) {
                menu->Draw();
            } else {
                Disable();
            }
        }
        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void Renderer::Enable() noexcept
    {
        InputBlocker::SetBlocked();
        _isEnable.store(true);
    }

    void Renderer::Disable() noexcept
    {
        _isEnable.store(false);
        InputBlocker::TryWantUnblock();
    }

    void Renderer::Load()
    {
        std::shared_lock configLock{ Configuration::Mutex() };
        std::shared_lock transLock{ Translation::Mutex() };

        fonts.Load();
        styles.Load();
        texts.Load();

        _configVersion = Configuration::Version();
        _transVersion = Translation::Version();

        SKSE::log::debug("Renderer: Upgrade to Configuration Version {}.", _configVersion);
        SKSE::log::debug("Renderer: Upgrade to Translation Version {}.", _transVersion);
    }

    Renderer Renderer::_singleton;
}
