#include "crash_handler.h"
#include <cstdio>
#include <cwchar>

static const wchar_t* GetExceptionDescription(DWORD code) {
    switch (code) {
        case 0xC0000005: return L"STATUS_ACCESS_VIOLATION (Violação de Acesso à Memória)";
        case 0xC00000FD: return L"STATUS_STACK_OVERFLOW (Estouro de Pilha / Recursão Excessiva)";
        case 0xC000001D: return L"STATUS_ILLEGAL_INSTRUCTION (Instrução Inválida)";
        case 0xC000008C: return L"STATUS_ARRAY_BOUNDS_EXCEEDED (Índice fora do limite do array)";
        case 0xC000008E: return L"STATUS_FLOAT_DIVIDE_BY_ZERO (Divisão por zero em ponto flutuante)";
        case 0xC0000094: return L"STATUS_INTEGER_DIVIDE_BY_ZERO (Divisão por zero inteira)";
        case 0xC0000002: return L"STATUS_DATATYPE_MISALIGNMENT (Alinhamento incorreto de dados)";
        case 0xC000041D: return L"STATUS_FATAL_USER_CALLBACK_EXCEPTION (Falha Crítica em Callback do Windows)";
        default:          return L"EXCEÇÃO_DESCONHECIDA";
    }
}

static LONG WINAPI GlobalExceptionHandler(EXCEPTION_POINTERS *pExInfo) {
    DWORD code = pExInfo->ExceptionRecord->ExceptionCode;
    PVOID addr = pExInfo->ExceptionRecord->ExceptionAddress;
    const wchar_t *desc = GetExceptionDescription(code);

    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // Grava log detalhado do crash no diretório atual
    FILE *f = nullptr;
    _wfopen_s(&f, L"sizescan_crash.log", L"w, ccs=UTF-8");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fwprintf(f, L"===========================================================\n");
        fwprintf(f, L"          RELATÓRIO DE ERRO CRÍTICO - SIZE-SCAN PLUS       \n");
        fwprintf(f, L"===========================================================\n");
        fwprintf(f, L"Data/Hora: %02d/%02d/%04d %02d:%02d:%02d.%03d\n", 
            st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        fwprintf(f, L"Executável: %s\n", exePath);
        fwprintf(f, L"Código do Erro: 0x%08X (%s)\n", code, desc);
        fwprintf(f, L"Endereço da Falha: 0x%p\n", addr);
        if (pExInfo->ContextRecord) {
#if defined(_M_X64) || defined(__x86_64__)
            fwprintf(f, L"\nRegistradores x64:\n");
            fwprintf(f, L"RAX: 0x%016llX  RBX: 0x%016llX\n", pExInfo->ContextRecord->Rax, pExInfo->ContextRecord->Rbx);
            fwprintf(f, L"RCX: 0x%016llX  RDX: 0x%016llX\n", pExInfo->ContextRecord->Rcx, pExInfo->ContextRecord->Rdx);
            fwprintf(f, L"RSI: 0x%016llX  RDI: 0x%016llX\n", pExInfo->ContextRecord->Rsi, pExInfo->ContextRecord->Rdi);
            fwprintf(f, L"RSP: 0x%016llX  RBP: 0x%016llX\n", pExInfo->ContextRecord->Rsp, pExInfo->ContextRecord->Rbp);
            fwprintf(f, L"RIP: 0x%016llX  EFLAGS: 0x%08X\n", pExInfo->ContextRecord->Rip, pExInfo->ContextRecord->EFlags);
#endif
        }
        fwprintf(f, L"===========================================================\n");
        fclose(f);
    }

    wchar_t msg[1024];
    swprintf_s(msg, 1024,
        L"O SIZE-SCAN PLUS encontrou um erro crítico e precisou ser encerrado.\n\n"
        L"Código do Erro: 0x%08X\n"
        L"Tipo: %s\n"
        L"Endereço: 0x%p\n\n"
        L"Um arquivo de diagnóstico foi gravado em:\n"
        L"sizescan_crash.log",
        code, desc, addr
    );

    MessageBoxW(NULL, msg, L"SIZE-SCAN PLUS - Falha Crítica Detectada", MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);

    return EXCEPTION_EXECUTE_HANDLER;
}

void InstallCrashHandler() {
    SetUnhandledExceptionFilter(GlobalExceptionHandler);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
}
