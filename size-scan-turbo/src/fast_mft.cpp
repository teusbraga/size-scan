#include "fast_mft.h"
#include <wchar.h>
#include <string.h>

static inline wchar_t to_lower_fast(wchar_t c) {
    if (c >= L'A' && c <= L'Z') return c + 32;
    return c;
}

FileCategory FastClassifyFileTurbo(const wchar_t *filename) {
    if (!filename) return CAT_OTHER;

    // Achar o ultimo ponto '.'
    const wchar_t *dot = wcsrchr(filename, L'.');
    if (!dot) return CAT_OTHER;

    size_t len = wcslen(dot);
    if (len >= 2 && len <= 4) {
        // Empacotar ate 4 wchar_t em um uint64_t de 64 bits para o Assembly x64
        uint64_t packed = 0;
        wchar_t *pChars = (wchar_t*)&packed;
        for (size_t i = 0; i < len; i++) {
            pChars[i] = to_lower_fast(dot[i]);
        }
        uint32_t cat = asm_classify_ext_hash(packed);
        if (cat < CAT_OTHER) {
            return (FileCategory)cat;
        }
    }

    // Extensoes comuns com 5 ou mais caracteres
    // Video
    if (_wcsicmp(dot, L".webm") == 0 || _wcsicmp(dot, L".mpeg") == 0) return CAT_VIDEO;
    // Imagem
    if (_wcsicmp(dot, L".jpeg") == 0 || _wcsicmp(dot, L".tiff") == 0 || _wcsicmp(dot, L".webp") == 0) return CAT_IMAGE;
    // Audio
    if (_wcsicmp(dot, L".flac") == 0 || _wcsicmp(dot, L".opus") == 0) return CAT_AUDIO;
    // Compactados
    if (_wcsicmp(dot, L".tar.gz") == 0 || _wcsicmp(dot, L".7z") == 0 || _wcsicmp(dot, L".vmdk") == 0) return CAT_ARCHIVE;
    // Documentos
    if (_wcsicmp(dot, L".docx") == 0 || _wcsicmp(dot, L".xlsx") == 0 || _wcsicmp(dot, L".pptx") == 0 || _wcsicmp(dot, L".epub") == 0) return CAT_DOC;
    // Executaveis
    if (_wcsicmp(dot, L".appx") == 0) return CAT_EXEC;
    // Codigo
    if (_wcsicmp(dot, L".json") == 0 || _wcsicmp(dot, L".yaml") == 0 || _wcsicmp(dot, L".html") == 0 || _wcsicmp(dot, L".java") == 0 || _wcsicmp(dot, L".ipynb") == 0) return CAT_CODE;

    return CAT_OTHER;
}
