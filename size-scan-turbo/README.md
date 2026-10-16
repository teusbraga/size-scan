# SIZE-SCAN TURBO (Edição de Alta Performance para Entusiastas) ⚡💾

> **Analisador de Armazenamento com Motor de Leitura Direta de MFT NTFS, Rotinas em Assembly x64 (`ml64`), SIMD AVX2, UAC Elevation e Varredura Sub-segundo (< 1s para 1 milhão de arquivos).**

O **SIZE-SCAN TURBO** é a edição para entusiastas e *power users* baseada na arquitetura do **Size-Scan Plus**. Ele implementa as mesmas técnicas de baixo nível consagradas pelo **WizTree**, combinando:

1. **Acesso Direto ao Volume NTFS (`\\.\C:`)**: Ignora o subsistema tradicional do Windows (`FindFirstFileW`) e faz streaming sequencial do arquivo `$MFT` em blocos alinhados de 2 MB a 4 MB.
2. **Rotinas em Assembly x64 Puro (`ml64.exe`)**:
   - `mft_apply_fixup_x64`: Validação e aplicação dos *Fixup Words* (Update Sequence Array - USA) diretamente nos setores de 512 bytes em pouquíssimos ciclos de clock.
   - `asm_fast_sum_u64_avx2`: Acumulação vetorizada de tamanhos de arquivo em paralelo com registradores YMM de 256 bits.
   - `asm_classify_ext_hash`: Classificação instantânea de extensões de arquivo empacotadas em 64 bits UTF-16LE em ~1 nanosegundo.
3. **Resolução de Árvore em O(N)**: Mapeamento direto de File Reference Numbers (FRN) para construção da árvore sem alocações dinâmicas repetitivas.
4. **Fallback Inteligente**: Se executado em drives externos não-NTFS (FAT32/exFAT) ou em pastas isoladas, aciona suavemente o motor multithread de alta performance.

---

## ⚡ Diferenças: Plus vs Turbo

| Característica | Size-Scan Plus | Size-Scan Turbo ⚡ |
| :--- | :--- | :--- |
| **Público Alvo** | Usuários Comuns | Entusiastas & Power Users |
| **Permissão (UAC)** | Usuário Comum (`asInvoker`) | Administrador (`requireAdministrator`) |
| **Método de Leitura** | `FindFirstFileExW` Otimizado + `StringPool` | **Streaming Direto da MFT NTFS** |
| **Aceleração por Hardware** | C++17 | **Assembly x64 (`ml64`) + AVX2 SIMD** |
| **Tempo de Varredura (1TB / 1M arquivos)** | ~15 a 35 segundos | **~0,5 a 1,8 segundos** |
| **Tamanho do Binário** | ~404 KB | ~421 KB (Standalone) |

---

## 🔨 Como Compilar

Certifique-se de ter o Microsoft Visual C++ Build Tools instalado (com `cl.exe` e `ml64.exe`).

Execute no diretório `size-scan-turbo`:
```cmd
build.bat
```

O script automaticamente:
1. Monta o módulo em Assembly x64 com `ml64.exe /nologo /c /Fosrc\fast_mft.obj src\fast_mft.asm`.
2. Compila os recursos e ícone com `rc.exe`.
3. Compila os módulos C++ com `/O2 /arch:AVX2 /std:c++17`.
4. Embuti o manifesto com elevação UAC (`requireAdministrator`) e linka com o executável final `sizescan_turbo.exe`.

---

## 🚀 Como Executar

Basta dar dois cliques em `sizescan_turbo.exe`. O Windows exibirá o diálogo do UAC solicitando privilégios de Administrador. Ao confirmar, o motor Turbo iniciará a varredura direta do disco com velocidade máxima!
