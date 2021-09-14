#include "pch.h"
#include <XamlWin32Helpers.h>
#include "WebView2.h"



struct AppWindow
{
    static inline const auto contentText = LR"(
<Page
    xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
    xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
    xmlns:d="http://schemas.microsoft.com/expression/blend/2008"
    xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006">
    <NavigationView>
        <NavigationView.MenuItems>
            <NavigationViewItem Content="Startup">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE7B5;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Interaction">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE7C9;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Appearance">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE771;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Color schemes">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE790;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Rendering">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE7F8;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

            <NavigationViewItem Content="Actions">
                <NavigationViewItem.Icon>
                    <FontIcon Glyph="&#xE765;" />
                </NavigationViewItem.Icon>
            </NavigationViewItem>

        </NavigationView.MenuItems>
    </NavigationView>
</Page>
)";

    void CreateWebView()
    {
        WNDCLASSEXW wcex{ sizeof(wcex) };
        wcex.lpszClassName = L"WebViewHostChildClass";
        wcex.hInstance = wil::GetModuleInstanceHandle();
        wcex.lpfnWndProc = DefWindowProcW;

        RegisterClassExW(&wcex);

        RECT rcClient;
        GetClientRect(m_window.get(), &rcClient);
        rcClient.right -= 100;
        rcClient.left += 100;
        rcClient.top += 100;
        rcClient.bottom -= 100;
        

        m_webviewHostChild = CreateWindowExW(WS_EX_LAYERED, L"WebViewHostChildClass", nullptr, WS_CHILD | WS_VISIBLE, 100, 100, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, m_window.get(), nullptr, wil::GetModuleInstanceHandle(), nullptr);

        THROW_IF_FAILED(CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

                    // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                    env->CreateCoreWebView2Controller(m_webviewHostChild, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                            if (controller != nullptr) {
                                m_webViewController = controller;
                                m_webViewController->get_CoreWebView2(&m_webView);
                            }

                            // Add a few settings for the webview
                            // The demo step is redundant since the values are the default settings
                            ICoreWebView2Settings* Settings;
                            m_webView->get_Settings(&Settings);
                            Settings->put_IsScriptEnabled(TRUE);
                            Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                            Settings->put_IsWebMessageEnabled(TRUE);

                            // Resize WebView to fit the bounds of the parent window
                            RECT bounds;
                            GetClientRect(m_window.get(), &bounds);
                            bounds.right -= 100;
                            bounds.left += 100;
                            bounds.top += 100;
                            bounds.bottom -= 100;
                            m_webViewController->put_Bounds(bounds);

                            // Schedule an async task to navigate to Bing
                            m_webView->Navigate(L"https://www.bing.com/");

                            m_webViewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT);

                            // Step 4 - Navigation events

                            // Step 5 - Scripting

                            // Step 6 - Communication between host and web content

                            PostMessage(m_window.get(), WM_USER, 0, 0);

                            return S_OK;
                        }).Get());
                    return S_OK;
                }).Get()));
    }



    void CreateXAML()
    {
        WNDCLASSEXW wcex{ sizeof(wcex) };
        wcex.lpszClassName = L"XamlHostChildClass";
        wcex.hInstance = wil::GetModuleInstanceHandle();
        wcex.lpfnWndProc = DefWindowProcW;

        RegisterClassExW(&wcex);

        RECT rcClient;
        GetClientRect(m_window.get(), &rcClient);

        m_xamlHostChild = CreateWindowExW(WS_EX_LAYERED, L"XamlHostChildClass", nullptr, WS_CHILD | WS_VISIBLE, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, m_window.get(), nullptr, wil::GetModuleInstanceHandle(), nullptr);

        m_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();

        auto interop = m_xamlSource.as<IDesktopWindowXamlSourceNative>();
        THROW_IF_FAILED(interop->AttachToWindow(m_xamlHostChild));
        THROW_IF_FAILED(interop->get_WindowHandle(&m_xamlSourceWindow));

        // When this fails look in the debug output window, it shows the line and offset
        // that has the parsing problem.
        auto content = winrt::Windows::UI::Xaml::Markup::XamlReader::Load(contentText).as<winrt::Windows::UI::Xaml::UIElement>();

        m_pointerPressedRevoker = content.PointerPressed(winrt::auto_revoke, [](auto&&, auto&& args)
            {
                auto poitnerId = args.Pointer().PointerId();
            });

        m_xamlSource.Content(content);

        // TODO: register for tab-out events, transfer focus to webview in handler
        //m_xamlSource.TakeFocusRequested()
        

        SetWindowPos(m_xamlSourceWindow, HWND_BOTTOM, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, SWP_SHOWWINDOW | SWP_NOACTIVATE);

    }

    LRESULT OnCreate()
    {
        CreateWebView();

        //CreateXAML();

        return 0;
    }

    LRESULT OnSize(WPARAM /* SIZE_XXX */wparam, LPARAM /* x, y */ lparam)
    {
        const auto dx = LOWORD(lparam), dy = HIWORD(lparam);
        SetWindowPos(m_xamlSourceWindow, HWND_BOTTOM, 0, 0, dx, dy, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        SetWindowPos(m_xamlHostChild, HWND_BOTTOM, 0, 0, dx, dy, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        return 0;
    }

    LRESULT OnDestroy()
    {
        // Since the xaml rundown is async and requires message dispatching,
        // run it down here while the message loop is still running.
        // Work around http://task.ms/33900412, to be fixed
        m_xamlSource.Close();
        PostQuitMessage(0);
        return 0;
    }

    LRESULT MessageHandler(UINT message, WPARAM wparam, LPARAM lparam) noexcept
    {
        switch (message)
        {
        case WM_CREATE:
            return OnCreate();

        case WM_SIZE:
            return OnSize(wparam, lparam);

        case WM_DESTROY:
            return OnDestroy();

        case WM_USER:
            CreateXAML();
            return 0;
        }
        return DefWindowProcW(m_window.get(), message, wparam, lparam);
    }

    void Show(int nCmdShow)
    {
        const PCWSTR className = L"Win32XamlAppWindow";
        RegisterWindowClass<AppWindow>(className);

        auto hwnd = CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP, className, L"Win32 Xaml App", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, wil::GetModuleInstanceHandle(), this);
        THROW_LAST_ERROR_IF(!hwnd);

        ShowWindow(m_window.get(), nCmdShow);
        UpdateWindow(m_window.get());
        MessageLoop();
    }

    void MessageLoop()
    {
        MSG msg = {};
        while (GetMessageW(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    wil::unique_hwnd m_window;

    HWND m_xamlHostChild{};
    HWND m_xamlSourceWindow{}; // This is owned by m_xamlSource, destroyed when Close() is called.

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };

    winrt::Windows::UI::Xaml::UIElement::PointerPressed_revoker m_pointerPressedRevoker;

    HWND m_webviewHostChild{};
    Microsoft::WRL::ComPtr<ICoreWebView2Controller> m_webViewController;
    Microsoft::WRL::ComPtr<ICoreWebView2> m_webView;
};

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    auto coInit = wil::CoInitializeEx(COINIT_APARTMENTTHREADED);
    std::make_unique<AppWindow>()->Show(nCmdShow);
    return 0;
}
