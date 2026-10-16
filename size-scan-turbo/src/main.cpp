#include <windows.h>
#include <gdiplus.h>
#include <objbase.h>
#include "native_ui.h"
#include "crash_handler.h"

#pragma comment(lib, "gdiplus.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Instala capturador avançado de falhas e logs de diagnóstico com código de erro
    InstallCrashHandler();

    // Habilita Per-Monitor V2 DPI Awareness para texto e gráficos nítidos
    typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        SetProcessDpiAwarenessContextProc pSetDpi = 
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetDpi) {
            pSetDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    // Inicializa COM para caixas de diálogo do Windows Explorer
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Inicializa GDI+ para gráficos vetoriais de alto desempenho e baixo consumo
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    if (!g_ui.Create(hInstance, nCmdShow)) {
        MessageBoxW(NULL, L"Não foi possível inicializar a interface do Size Scan Plus.", L"Erro", MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        if (SUCCEEDED(hr)) CoUninitialize();
        return 1;
    }

    // Loop de mensagens Win32
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Encerrar threads ativas e destruir buffers GDI+ ANTES do GdiplusShutdown
    g_model.Shutdown();
    g_ui.Destroy();

    Gdiplus::GdiplusShutdown(gdiplusToken);
    if (SUCCEEDED(hr)) {
        CoUninitialize();
    }

    return (int)msg.wParam;
}
