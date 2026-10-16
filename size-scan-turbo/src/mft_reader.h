#ifndef MFT_READER_H
#define MFT_READER_H

#include <windows.h>
#include <stdint.h>
#include "scanner_fast.h"

class MftReader {
public:
    // Tenta elevar privilegios de volume (SeManageVolumePrivilege e SeBackupPrivilege)
    static bool EnablePrivileges();

    // Verifica se temos permissao de acesso ao volume (Admin)
    static bool CanAccessVolume(const wchar_t *drivePath);

    // Executa a leitura ultrarrápida da MFT diretamente do volume NTFS
    static bool ScanVolumeMFT(
        const wchar_t *driveLetter,
        FastScanResult &out_result,
        volatile bool *cancel_flag,
        volatile bool *is_running,
        ScanProgressFn progress_fn = nullptr,
        void *user_data = nullptr
    );
};

#endif // MFT_READER_H
