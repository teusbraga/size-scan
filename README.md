# SIZE-SCAN 🚀💾

> **Suite Ultrarrápida e Ultraleve de Análise e Monitoramento de Armazenamento em Disco para Windows (x64 Nativo em C/C++ e Assembly)**

O **SIZE-SCAN** foi projetado para entregar máxima velocidade de varredura, consumo mínimo de memória RAM e uma interface gráfica moderna e fluida, sem depender de Electron, .NET, Python ou frameworks pesados.

---

## 📦 Edições Disponíveis

Este repositório contém duas edições independentes e otimizadas para diferentes cenários:

| Recurso | 🚀 SIZE-SCAN PLUS (`size-scan-plus/`) | ⚡ SIZE-SCAN TURBO (`size-scan-turbo/`) |
| :--- | :--- | :--- |
| **Motor de Varredura** | Multithreaded I/O com Win32 API (`FindFirstFileExW`) | Leitura direta da **MFT (NTFS Master File Table)** com fallback Win32 |
| **Aceleração por Hardware** | C++ otimizado em nível `/O2` | **Assembly x64 puro (`fast_mft.asm`) + AVX2 / SIMD** |
| **Velocidade de Leitura** | Até centenas de milhares de arquivos/segundo | **Milhões de arquivos em segundos** em discos NTFS |
| **Privilégios Necessários** | Usuário padrão (não requer Administrador) | Administrador (para acesso direto à MFT de disco bruto) ou padrão |
| **Interface Gráfica** | Win32 + GDI+ com Double-Buffering | Win32 + GDI+ com Double-Buffering |
| **Menu Bar Clássica** | Sim (Gavetinhas padrão Windows com atalhos) | Sim (Gavetinhas padrão Windows com atalhos) |
| **Acessibilidade de Fontes**| `Ctrl +` / `Ctrl -` / `Ctrl 0` e `Ctrl + Scroll` | `Ctrl +` / `Ctrl -` / `Ctrl 0` e `Ctrl + Scroll` |
| **Temas & Idiomas** | Modo Claro / Escuro + PT, EN, ES | Modo Claro / Escuro + PT, EN, ES |
| **Consumo de Memória** | ~35 MB a 60 MB RAM | ~40 MB a 80 MB RAM |

---

## 🚀 1. SIZE-SCAN PLUS

A edição **Plus** é ideal para o uso diário em qualquer computador com Windows 10 ou Windows 11. Não necessita de privilégios de administrador e analisa qualquer partição ou pasta instantaneamente.

### Principais Funcionalidades:
- **Visual Moderno e Intuitivo**: Gráfico de rosca (*Donut Chart*), cards de categorias clicáveis com filtro imediato e medidor visual da capacidade do disco.
- **Detecção de Espaço Real vs. Clusters**: Alternância entre tamanho lógico (bytes) e tamanho físico real alocado no disco (clusters/blocos NTFS, incluindo arquivos esparsos).
- **Filtro de Não-Redundância**: Esconde subpastas secundárias se a pasta pai já está listada entre as mais pesadas, evitando contagem duplicada visual.
- **Navegação Rápida com Explorer**: Botão `[ Abrir Pasta ]` que abre o Windows Explorer já com o arquivo ou pasta selecionado.
- **Multilíngue (i18n)**: Suporte completo a Português do Brasil, Inglês e Espanhol com bandeirinhas em pixel art e sincronização em tempo real.
- **Menu Bar Nativa**: Menus clássicos suspensos do Windows com atalhos rápidos (`Ctrl+O`, `F5`, `F1`, etc.).

### Como Compilar:
```cmd
cd size-scan-plus
build.bat
```
Executável gerado: `size-scan-plus\sizescan_plus.exe`

---

## ⚡ 2. SIZE-SCAN TURBO

A edição **Turbo** é voltada para analistas, administradores e entusiastas que buscam a velocidade máxima absoluta possível no Windows, lendo diretamente os registros da `$MFT` (Master File Table) do sistema de arquivos NTFS via chamadas de baixo nível e rotinas em Assembly x64 (`ml64.exe`).

### Diferenciais da Edição Turbo:
- **Leitura Direta da MFT**: Ignora a árvore de arquivos e lê sequencialmente os metadados do volume NTFS em blocos contínuos.
- **Rotinas em Assembly x64**: Parser de atributos NTFS e decodificação de strings UTF-16 vetorizadas em ASM.
- **Fallback Automático**: Caso seja executado sem privilégios de administrador ou em unidades de rede/FAT32, chaveia suavemente para o motor multithread do Win32.

### Como Compilar:
```cmd
cd size-scan-turbo
build.bat
```
Executável gerado: `size-scan-turbo\sizescan_turbo.exe`

---

## 🛠️ Requisitos de Sistema

- **Sistema Operacional**: Windows 10 ou Windows 11 (64-bit)
- **Compilador (para recompilar)**: Microsoft Visual Studio (MSVC v143+ / Visual Studio Build Tools 2022 ou superior) com suporte a x64.
- **Dependências Externas**: Nenhuma! Utiliza apenas as APIs nativas do Windows (`user32.lib`, `gdiplus.lib`, `shell32.lib`, `dwmapi.lib`, `advapi32.lib`, `ole32.lib`).

---

## 📄 Licença

Distribuído sob a licença MIT. Consulte o arquivo [LICENSE](LICENSE) para mais detalhes.
