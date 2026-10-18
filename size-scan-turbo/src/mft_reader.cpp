#include "mft_reader.h"
#include "fast_mft.h"
#include <winioctl.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <execution>

#pragma pack(push, 1)

// Cabecalho padrao do registro MFT de 1024 bytes
struct MFT_RECORD_HEADER {
    DWORD magic;            // 'FILE' (0x454C4946) ou 'BAAD'
    WORD  updateSeqOffset;  // Offset para Update Sequence Array (USA)
    WORD  updateSeqSize;    // Tamanho do USA em WORDs
    UINT64 lsn;             // Log File Sequence Number
    WORD  sequenceNumber;   // Numero de sequencia
    WORD  hardLinkCount;    // Contagem de hardlinks
    WORD  firstAttrOffset;  // Offset do primeiro atributo
    WORD  flags;            // 0x01 = Em Uso, 0x02 = Diretorio
    DWORD bytesUsed;        // Tamanho real usado no registro
    DWORD bytesAllocated;   // Tamanho alocado (1024)
    UINT64 baseRecordRef;   // Referencia do registro base
    WORD  nextAttrId;       // Proximo ID de atributo
};

// Cabecalho generico de atributo NTFS
struct NTFS_ATTR_HEADER {
    DWORD type;             // 0x10=$STANDARD_INFO, 0x30=$FILE_NAME, 0x80=$DATA
    DWORD length;           // Comprimento total deste atributo
    BYTE  nonResident;      // 0 = Residente, 1 = Nao-residente
    BYTE  nameLength;       // Comprimento do nome do atributo (se houver)
    WORD  nameOffset;       // Offset do nome
    WORD  flags;            // Flags (comprimido, esparso, criptografado)
    WORD  id;               // ID do atributo
};

// Cabecalho de atributo residente
struct NTFS_ATTR_RESIDENT {
    NTFS_ATTR_HEADER header;
    DWORD valueLength;      // Tamanho do conteudo
    WORD  valueOffset;      // Offset do conteudo relativo ao inicio do atributo
    BYTE  indexedFlag;      // 1 se indexado
    BYTE  padding;
};

// Cabecalho de atributo nao-residente
struct NTFS_ATTR_NONRESIDENT {
    NTFS_ATTR_HEADER header;
    UINT64 startVcn;        // Starting Virtual Cluster Number
    UINT64 lastVcn;         // Last Virtual Cluster Number
    WORD  dataRunsOffset;   // Offset do run-list relativo ao inicio do atributo
    WORD  compressionUnit;  // Unidade de compressao
    DWORD padding;
    UINT64 allocatedSize;   // Tamanho alocado no disco
    UINT64 realSize;        // Tamanho logico do arquivo
    UINT64 initializedDataSize;
};

// Conteudo do atributo $FILE_NAME (0x30)
struct NTFS_FILE_NAME_ATTR {
    UINT64 parentDirectoryRef;  // Record Number do diretorio pai (48-bit)
    UINT64 creationTime;
    UINT64 alterTime;
    UINT64 mftChangeTime;
    UINT64 readTime;
    UINT64 allocatedSize;       // Tamanho alocado
    UINT64 realSize;            // Tamanho real
    DWORD  fileFlags;           // Flags (DIRECTORY, HIDDEN, etc.)
    DWORD  reparseValue;
    BYTE   fileNameLength;      // Tamanho em wchar_t
    BYTE   namespaceType;       // 0=POSIX, 1=Win32, 2=DOS, 3=Win32&DOS
    WCHAR  fileName[1];         // Nome do arquivo
};

#pragma pack(pop)

// Extensao de cluster da MFT
struct MftExtent {
    int64_t startLcn;
    int64_t clusterCount;
};

bool MftReader::EnablePrivileges() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    auto setPriv = [hToken](const wchar_t *privName) {
        LUID luid;
        if (!LookupPrivilegeValueW(NULL, privName, &luid)) return;
        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    };

    setPriv(SE_MANAGE_VOLUME_NAME);
    setPriv(SE_BACKUP_NAME);
    setPriv(SE_RESTORE_NAME);

    CloseHandle(hToken);
    return true;
}

bool MftReader::CanAccessVolume(const wchar_t *drivePath) {
    if (!drivePath || wcslen(drivePath) < 1) return false;
    EnablePrivileges();

    wchar_t volName[64];
    swprintf_s(volName, 64, L"\\\\.\\%c:", drivePath[0]);

    HANDLE hVol = CreateFileW(
        volName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hVol == INVALID_HANDLE_VALUE) {
        return false;
    }

    CloseHandle(hVol);
    return true;
}

// Decodifica a lista de execucao (run-list) de clusters da MFT
static void ParseDataRuns(const BYTE *runData, size_t maxLen, int64_t startVcn, std::vector<MftExtent> &outExtents) {
    size_t offset = 0;
    int64_t currentLcn = 0;

    while (offset < maxLen && runData[offset] != 0) {
        BYTE b = runData[offset++];
        int lenBytes = b & 0x0F;
        int offBytes = (b >> 4) & 0x0F;

        if (lenBytes == 0 || offset + lenBytes + offBytes > maxLen) break;

        int64_t runLength = 0;
        for (int i = 0; i < lenBytes; i++) {
            runLength |= ((int64_t)runData[offset + i]) << (i * 8);
        }
        offset += lenBytes;

        int64_t runOffset = 0;
        if (offBytes > 0) {
            for (int i = 0; i < offBytes; i++) {
                runOffset |= ((int64_t)runData[offset + i]) << (i * 8);
            }
            // Sinal estendido para negativo (saltos para tras no disco)
            if (runData[offset + offBytes - 1] & 0x80) {
                for (int i = offBytes; i < 8; i++) {
                    runOffset |= ((int64_t)0xFF) << (i * 8);
                }
            }
            offset += offBytes;
            currentLcn += runOffset;
        }

        if (runLength > 0 && currentLcn > 0) {
            MftExtent ext;
            ext.startLcn = currentLcn;
            ext.clusterCount = runLength;
            outExtents.push_back(ext);
        }
    }
}

// Registro interno rapido em memoria para indexacao O(1)
struct TempItem {
    uint64_t parentFRN;
    const wchar_t *name;
    uint64_t realSize;
    uint64_t allocSize;
    bool isDirectory;
    bool inUse;
};

bool MftReader::ScanVolumeMFT(
    const wchar_t *driveLetter,
    FastScanResult &out_result,
    volatile bool *cancel_flag,
    volatile bool *is_running,
    ScanProgressFn progress_fn,
    void *user_data
) {
    if (!driveLetter) return false;
    EnablePrivileges();

    wchar_t volName[64];
    swprintf_s(volName, 64, L"\\\\.\\%c:", driveLetter[0]);

    HANDLE hVol = CreateFileW(
        volName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (hVol == INVALID_HANDLE_VALUE) {
        return false;
    }

    NTFS_VOLUME_DATA_BUFFER ntfsData = { 0 };
    DWORD bytesRet = 0;
    if (!DeviceIoControl(hVol, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &ntfsData, sizeof(ntfsData), &bytesRet, NULL)) {
        CloseHandle(hVol);
        return false;
    }

    uint32_t clusterSize = ntfsData.BytesPerCluster;
    uint32_t bytesPerRecord = ntfsData.BytesPerFileRecordSegment ? ntfsData.BytesPerFileRecordSegment : 1024;
    out_result.cluster_size = clusterSize;

    // Espaco em disco do volume
    ULARGE_INTEGER freeBytes, totalBytes, totalFreeBytes;
    wchar_t rootPath[8] = { driveLetter[0], L':', L'\\', L'\0' };
    if (GetDiskFreeSpaceExW(rootPath, &freeBytes, &totalBytes, &totalFreeBytes)) {
        out_result.disk_total_bytes = totalBytes.QuadPart;
        out_result.disk_free_bytes = freeBytes.QuadPart;
        out_result.disk_used_bytes = (totalBytes.QuadPart >= freeBytes.QuadPart) 
                                     ? (totalBytes.QuadPart - freeBytes.QuadPart) : 0;
    }

    // Leitura do Registro 0 da MFT para obter todos os clusters de dados do proprio $MFT
    int64_t initialMftByteOffset = ntfsData.MftStartLcn.QuadPart * clusterSize;
    LARGE_INTEGER liOffset;
    liOffset.QuadPart = initialMftByteOffset;
    if (!SetFilePointerEx(hVol, liOffset, NULL, FILE_BEGIN)) {
        CloseHandle(hVol);
        return false;
    }

    // Ler primeiro cluster (que contem o Registro 0)
    std::vector<BYTE> firstCluster(clusterSize);
    DWORD bytesRead = 0;
    if (!ReadFile(hVol, firstCluster.data(), clusterSize, &bytesRead, NULL) || bytesRead < bytesPerRecord) {
        CloseHandle(hVol);
        return false;
    }

    // Aplicar fixup do registro 0 usando Assembly x64
    mft_apply_fixup_x64(firstCluster.data());

    // Localizar atributo $DATA (0x80) do Registro 0 para obter o run-list completo da MFT
    std::vector<MftExtent> mftExtents;
    MFT_RECORD_HEADER *pRecord0 = (MFT_RECORD_HEADER*)firstCluster.data();
    DWORD attrOffset = pRecord0->firstAttrOffset;

    while (attrOffset + sizeof(NTFS_ATTR_HEADER) <= bytesPerRecord) {
        NTFS_ATTR_HEADER *attr = (NTFS_ATTR_HEADER*)(firstCluster.data() + attrOffset);
        if (attr->type == 0xFFFFFFFF || attr->length == 0) break;

        if (attr->type == 0x80 && attr->nonResident) {
            NTFS_ATTR_NONRESIDENT *nonRes = (NTFS_ATTR_NONRESIDENT*)attr;
            if (attrOffset + nonRes->dataRunsOffset < bytesPerRecord) {
                const BYTE *runData = (const BYTE*)attr + nonRes->dataRunsOffset;
                size_t maxLen = attr->length - nonRes->dataRunsOffset;
                ParseDataRuns(runData, maxLen, nonRes->startVcn, mftExtents);
            }
            break;
        }
        attrOffset += attr->length;
    }

    // Se nao encontrou run-list explicito, assume a MFT inicial contigua estimada pelo tamanho
    if (mftExtents.empty()) {
        MftExtent ext;
        ext.startLcn = ntfsData.MftStartLcn.QuadPart;
        ext.clusterCount = (ntfsData.MftValidDataLength.QuadPart + clusterSize - 1) / clusterSize;
        mftExtents.push_back(ext);
    }

    // Buffer de streaming alinhado de 2 MB para leitura ultra-rapida sequencial do disco
    const size_t CHUNK_SIZE = 2 * 1024 * 1024;
    std::vector<BYTE> chunkBuffer(CHUNK_SIZE);
    uint64_t currentFRN = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    int64_t totalMftBytes = 0;
    for (const auto &ext : mftExtents) {
        totalMftBytes += ext.clusterCount * clusterSize;
    }

    size_t estRecords = (bytesPerRecord > 0) ? (size_t)(totalMftBytes / bytesPerRecord) + 4096 : 1000000;
    std::vector<TempItem> records(estRecords);
    for (auto &r : records) {
        r.inUse = false;
    }
    size_t validRecordCount = 0;

    int64_t totalBytesReadAcrossExtents = 0;
    DWORD lastProgressTick = GetTickCount();

    for (const auto &ext : mftExtents) {
        if (cancel_flag && *cancel_flag) break;

        int64_t extentBytes = ext.clusterCount * clusterSize;
        int64_t extentOffset = ext.startLcn * clusterSize;

        liOffset.QuadPart = extentOffset;
        if (!SetFilePointerEx(hVol, liOffset, NULL, FILE_BEGIN)) continue;

        int64_t bytesRemaining = extentBytes;
        while (bytesRemaining > 0) {
            if (cancel_flag && *cancel_flag) break;

            DWORD toRead = (DWORD)min((int64_t)CHUNK_SIZE, bytesRemaining);
            if (!ReadFile(hVol, chunkBuffer.data(), toRead, &bytesRead, NULL) || bytesRead == 0) {
                break;
            }
            bytesRemaining -= bytesRead;
            totalBytesReadAcrossExtents += bytesRead;

            DWORD now = GetTickCount();
            if (now - lastProgressTick > 80) {
                lastProgressTick = now;
                float pct = (totalMftBytes > 0) ? ((float)totalBytesReadAcrossExtents / (float)totalMftBytes) * 45.0f : 20.0f;
                if (pct > 45.0f) pct = 45.0f;
                if (progress_fn) {
                    ScanProgressInfo info;
                    info.current_folder = L"⚡ Lendo tabela MFT do volume (Assembly x64)...";
                    info.progress_pct = pct;
                    info.total_files = validRecordCount;
                    info.total_folders = 0;
                    info.total_scanned_bytes = 0;
                    info.total_scanned_on_disk = 0;
                    progress_fn(info, user_data);
                }
            }

            size_t numRecords = bytesRead / bytesPerRecord;
            for (size_t r = 0; r < numRecords; r++, currentFRN++) {
                if (currentFRN >= records.size()) {
                    records.resize(currentFRN + 65536);
                }
                BYTE *pRec = chunkBuffer.data() + (r * bytesPerRecord);

                // Rotina em Assembly x64: valida assinatura 'FILE' e aplica os fixups de setor
                if (!mft_apply_fixup_x64(pRec)) {
                    continue;
                }

                MFT_RECORD_HEADER *hdr = (MFT_RECORD_HEADER*)pRec;
                // Registro em uso? 0x01 = IN_USE
                if (!(hdr->flags & 0x0001)) {
                    continue;
                }

                bool isDir = (hdr->flags & 0x0002) != 0;
                DWORD off = hdr->firstAttrOffset;

                const wchar_t *chosenName = nullptr;
                uint64_t parentFRN = 0;
                uint64_t realSize = 0;
                uint64_t allocSize = 0;
                BYTE chosenNamespace = 2; // Comeca com DOS para ser substituido por Win32 se houver

                while (off + sizeof(NTFS_ATTR_HEADER) <= bytesPerRecord) {
                    NTFS_ATTR_HEADER *attr = (NTFS_ATTR_HEADER*)(pRec + off);
                    if (attr->type == 0xFFFFFFFF || attr->length == 0 || off + attr->length > bytesPerRecord) {
                        break;
                    }

                    // Atributo 0x30 = $FILE_NAME
                    if (attr->type == 0x30 && !attr->nonResident) {
                        NTFS_ATTR_RESIDENT *res = (NTFS_ATTR_RESIDENT*)attr;
                        if (res->valueOffset + sizeof(NTFS_FILE_NAME_ATTR) <= attr->length) {
                            NTFS_FILE_NAME_ATTR *fn = (NTFS_FILE_NAME_ATTR*)((BYTE*)res + res->valueOffset);
                            // Evitar nomes puramente DOS 8.3 se ja tivermos nome Win32
                            if (fn->fileNameLength > 0 && (chosenName == nullptr || fn->namespaceType != 2)) {
                                chosenName = out_result.pool.AddString(fn->fileName, fn->fileNameLength);
                                parentFRN = fn->parentDirectoryRef & 0x0000FFFFFFFFFFFFULL;
                                chosenNamespace = fn->namespaceType;
                                if (realSize == 0) {
                                    realSize = fn->realSize;
                                    allocSize = fn->allocatedSize;
                                }
                            }
                        }
                    }
                    // Atributo 0x80 = $DATA (sobrescreve com tamanho real se for arquivo)
                    else if (attr->type == 0x80 && !isDir) {
                        if (attr->nonResident) {
                            NTFS_ATTR_NONRESIDENT *nonRes = (NTFS_ATTR_NONRESIDENT*)attr;
                            realSize = nonRes->realSize;
                            allocSize = nonRes->allocatedSize;
                        } else {
                            NTFS_ATTR_RESIDENT *res = (NTFS_ATTR_RESIDENT*)attr;
                            realSize = res->valueLength;
                            allocSize = (res->valueLength + clusterSize - 1) & ~(clusterSize - 1);
                        }
                    }

                    off += attr->length;
                }

                if (chosenName && chosenName[0] != L'\0') {
                    // Ignorar arquivos de sistema do NTFS ocultos (como $MFT, $LogFile, $Volume)
                    if (currentFRN < 16 && chosenName[0] == L'$') {
                        continue;
                    }

                    TempItem item;
                    item.parentFRN = parentFRN;
                    item.name = chosenName;
                    item.realSize = realSize;
                    item.allocSize = allocSize;
                    item.isDirectory = isDir;
                    item.inUse = true;
                    records[currentFRN] = item;
                    validRecordCount++;
                }
            }
        }
    }

    CloseHandle(hVol);

    if (validRecordCount == 0) {
        return false;
    }

    // Resolucao da arvore de pastas em O(N) com caminhos completos
    std::unordered_map<uint64_t, std::wstring> pathCache;
    pathCache.reserve(validRecordCount / 8);
    pathCache[5] = rootPath; // Registro 5 na MFT sempre e a raiz do disco (ex: C:\)

    auto buildPath = [&](uint64_t frn, auto &self) -> std::wstring {
        if (frn >= records.size()) return rootPath;
        auto it = pathCache.find(frn);
        if (it != pathCache.end()) return it->second;

        const auto &item = records[frn];
        if (!item.inUse || item.name == nullptr || frn == item.parentFRN) {
            return rootPath;
        }

        std::wstring parentPath = self(item.parentFRN, self);
        std::wstring full;
        if (!parentPath.empty() && parentPath.back() == L'\\') {
            full = parentPath + item.name;
        } else {
            full = parentPath + L"\\" + item.name;
        }
        pathCache[frn] = full;
        return full;
    };

    // Montar registros finais de arquivos e pastas no FastScanResult
    out_result.files.reserve(validRecordCount);
    out_result.folders.reserve(validRecordCount / 8);

    std::vector<uint32_t> folderIndexByFrn(records.size(), 0xFFFFFFFF);

    // 1. Primeiro passar as pastas para a lista
    for (size_t frn = 0; frn < records.size(); frn++) {
        if (cancel_flag && *cancel_flag) break;
        const auto &item = records[frn];
        if (item.inUse && item.isDirectory) {
            std::wstring folderPath = buildPath(frn, buildPath);
            FastFolderRecord fRec;
            fRec.folder_path = out_result.pool.AddString(folderPath.c_str());
            fRec.display_name = item.name;
            fRec.cumulative_size = 0;
            fRec.file_count = 0;
            fRec.subfolder_count = 0;
            fRec.is_redundant_child = false;

            folderIndexByFrn[frn] = (uint32_t)out_result.folders.size();
            out_result.folders.push_back(fRec);
        }
    }

    if (progress_fn) {
        ScanProgressInfo info;
        info.current_folder = L"⚡ Reconstruindo hierarquia de diretórios...";
        info.progress_pct = 50.0f;
        info.total_files = 0;
        info.total_folders = out_result.folders.size();
        info.total_scanned_bytes = 0;
        info.total_scanned_on_disk = 0;
        progress_fn(info, user_data);
    }

    // 2. Em seguida processar os arquivos com concatenação direta e zero alocações na heap
    for (size_t frn = 0; frn < records.size(); frn++) {
        if (cancel_flag && *cancel_flag) break;
        const auto &item = records[frn];
        if (item.inUse && !item.isDirectory) {
            uint32_t parentFolderIdx = (item.parentFRN < folderIndexByFrn.size()) ? folderIndexByFrn[item.parentFRN] : 0xFFFFFFFF;
            const wchar_t *parentPath = (parentFolderIdx != 0xFFFFFFFF) ? out_result.folders[parentFolderIdx].folder_path : rootPath;

            size_t parentLen = wcslen(parentPath);
            size_t nameLen = wcslen(item.name);

            FastFileRecord fileRec;
            fileRec.full_path = out_result.pool.AddPath(parentPath, parentLen, item.name, nameLen);
            fileRec.file_name = item.name;
            fileRec.size_bytes = item.realSize;
            fileRec.size_on_disk = item.allocSize;
            fileRec.last_modified = { 0, 0 };
            
            // Classificacao ultra rapida via Assembly x64
            fileRec.category = FastClassifyFileTurbo(item.name);

            out_result.cat_size[fileRec.category] += fileRec.size_bytes;
            out_result.cat_size_on_disk[fileRec.category] += fileRec.size_on_disk;
            out_result.cat_count[fileRec.category]++;

            out_result.total_scanned_bytes += fileRec.size_bytes;
            out_result.total_scanned_on_disk += fileRec.size_on_disk;
            out_result.total_files++;

            out_result.files.push_back(fileRec);

            // Acumular no pai direto se estiver registrado
            if (parentFolderIdx != 0xFFFFFFFF) {
                out_result.folders[parentFolderIdx].cumulative_size += fileRec.size_bytes;
                out_result.folders[parentFolderIdx].file_count++;
            }

            DWORD now = GetTickCount();
            if (now - lastProgressTick > 80) {
                lastProgressTick = now;
                float pct = 50.0f + ((float)out_result.total_files / (float)validRecordCount) * 48.0f;
                if (pct > 98.0f) pct = 98.0f;
                if (progress_fn) {
                    ScanProgressInfo info;
                    info.current_folder = fileRec.full_path;
                    info.progress_pct = pct;
                    info.total_files = out_result.total_files;
                    info.total_folders = out_result.folders.size();
                    info.total_scanned_bytes = out_result.total_scanned_bytes;
                    info.total_scanned_on_disk = out_result.total_scanned_on_disk;
                    progress_fn(info, user_data);
                }
            }
        }
    }

    out_result.total_folders = out_result.folders.size();

    // Notificar UI de que estamos organizando e classificando os resultados
    if (progress_fn) {
        ScanProgressInfo info;
        info.current_folder = L"📊 Organizando e classificando resultados...";
        info.progress_pct = 99.0f;
        info.total_files = out_result.total_files;
        info.total_folders = out_result.total_folders;
        info.total_scanned_bytes = out_result.total_scanned_bytes;
        info.total_scanned_on_disk = out_result.total_scanned_on_disk;
        progress_fn(info, user_data);
    }

    // Ordenação multi-core paralela com std::execution::par
    if (out_result.files.size() > 1) {
        std::sort(std::execution::par, out_result.files.begin(), out_result.files.end(), [](const FastFileRecord &a, const FastFileRecord &b) {
            return a.size_bytes > b.size_bytes;
        });
    }

    if (out_result.folders.size() > 1) {
        std::sort(std::execution::par, out_result.folders.begin(), out_result.folders.end(), [](const FastFolderRecord &a, const FastFolderRecord &b) {
            return a.cumulative_size > b.cumulative_size;
        });
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedSec = std::chrono::duration<double>(endTime - startTime).count();

    out_result.scan_progress_pct = 100.0f;

    if (progress_fn) {
        wchar_t msg[128];
        swprintf_s(msg, 128, L"🚀 MFT TURBO: %llu arquivos em %.2fs", out_result.total_files, elapsedSec);
        ScanProgressInfo info;
        info.current_folder = msg;
        info.progress_pct = 100.0f;
        info.total_files = out_result.total_files;
        info.total_folders = out_result.total_folders;
        info.total_scanned_bytes = out_result.total_scanned_bytes;
        info.total_scanned_on_disk = out_result.total_scanned_on_disk;
        progress_fn(info, user_data);
    }

    return true;
}
