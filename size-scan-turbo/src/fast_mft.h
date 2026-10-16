#ifndef FAST_MFT_H
#define FAST_MFT_H

#include <windows.h>
#include <stdint.h>
#include "../../types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Rotina em Assembly x64 puro (ml64.exe)
// Valida assinatura 'FILE', UpdateSequenceOffset e aplica fixups nos dois setores de 512 bytes
uint32_t mft_apply_fixup_x64(void *record_1024_bytes);

// Rotina SIMD AVX2 em Assembly x64 puro para somar vetores de inteiros de 64 bits em paralelo
uint64_t asm_fast_sum_u64_avx2(const uint64_t *values, size_t count);

// Rotina de classificacao ultra rapida de extensoes comuns via QWORD UTF-16
uint32_t asm_classify_ext_hash(uint64_t ext_packed_utf16);

#ifdef __cplusplus
}
#endif

// Helper C++ para classificar extensao em nanossegundos
FileCategory FastClassifyFileTurbo(const wchar_t *filename);

#endif // FAST_MFT_H
