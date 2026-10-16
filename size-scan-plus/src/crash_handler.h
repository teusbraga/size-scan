#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <windows.h>

// Instala o manipulador global de exceções para capturar qualquer crash crítico
// e exibir mensagem com código de erro detalhado + salvar 'sizescan_crash.log'
void InstallCrashHandler();

#endif // CRASH_HANDLER_H
