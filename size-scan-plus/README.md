# SIZE-SCAN PLUS (Edição Win32 Nativa Ultraleve) 🚀💾

> **Analisador de Armazenamento e Limpeza de Disco com Visual Nativo Comercial do Windows (Estilo Explorer / WizTree / Task Manager), 0% GPU, ~15 MB de RAM e Executável de 278 KB.**

O **SIZE-SCAN PLUS** foi reescrito em **C++ Win32 puro** com **Common Controls v6** (`SysListView32`, `SysTabControl32`, `DWM Dark Mode`). Ele combina a velocidade bruta do motor de varredura em **C puro** (`scanner.c`, `filters.c`, `types.h`) com uma interface idêntica aos utilitários comerciais nativos do Windows.

*(Nota: A versão anterior baseada em Dear ImGui + DirectX 11 foi preservada e arquivada intacta na pasta `size-scan-plus-imgui/`).*

---

## 🌟 Principais Recursos e Diferenciais

### 1. 🗂️ Filtro Inteligente de Subpastas Redundantes *(Novo)*
- **Eliminação de redundância em cascata**: Em árvores profundas (ex: `Jogos\League of Legends\Game\DATA\FINAL`), o aplicativo por padrão exibe apenas a pasta raiz mais pesada quando as filhas são subsequentes e contêm o mesmo tamanho.
- **Botão Alternador no Topo**: Alterne instantaneamente entre:
  - `[ Subpastas: Ocultas (Filtro Ativo) ]`: Mostra apenas pastas não-redundantes isoladas.
  - `[ Subpastas: Visíveis (Todas) ]`: Exibe todas as subpastas detalhadamente.

### 2. 💽 Modo de Tamanho: Real (Bytes) vs Em Disco (Clusters) *(Novo)*
- **Cálculo de alocação física**: Detecta automaticamente o tamanho dos clusters do volume NTFS/FAT32 com `GetDiskFreeSpaceW` (ex: 4096 bytes por cluster).
- **Botão Alternador no Topo**:
  - `[ Modo: Tamanho Real (Bytes) ]`: Tamanho lógico exato dos arquivos.
  - `[ Modo: Tamanho em Disco (Clusters) ]`: Espaço físico real ocupado pelos blocos no disco.

### 3. ⚡ Desempenho Ultraleve e Instantâneo
- **0% de Uso de GPU**: Não utiliza render loop contínuo de DirectX ou OpenGL; só atualiza quando há eventos do Windows ou novos dados de varredura.
- **Consumo de Memória Reduzido**: Apenas ~15 MB de RAM em repouso.
- **Binário Standalone**: Apenas 278 KB compilado, sem dependência de DLLs externas ou runtimes pesados.
- **Tipografia Nítida**: Suporte nativo a *Per-Monitor V2 DPI Awareness* e fontes Segoe UI com renderização ClearType.

### 4. 📋 Visual Explorer / WizTree com Abas e Tabelas Ordenáveis
- **Abas de Navegação**:
  - 📁 **Pastas Mais Pesadas**: Classificação hierárquica das maiores pastas, contagem de arquivos e subpastas.
  - 📄 **Maiores Arquivos**: Top arquivos mais pesados com suas categorias e caminhos absolutos.
  - 📊 **Categorias de Extensão**: Distribuição do volume escaneado entre Vídeos, Imagens, Áudios, Compactados, Documentos, Executáveis, Código Fonte e Outros.
- **Ordenação Instantânea**: Clique em qualquer cabeçalho de coluna para ordenar crescentemente ou decrescente.
- **Busca em Tempo Real**: Campo de pesquisa rápida que filtra instantaneamente por nome ou caminho.
- **Menu de Contexto Completo (Botão Direito)**:
  - *Abrir no Windows Explorer*
  - *Copiar Caminho*
  - *Escanear Somente Esta Pasta*
  - *Propriedades do Windows*
- **Duplo Clique**: Abre a pasta ou seleciona o arquivo diretamente no Explorador do Windows.

---

## 🖥️ Como Executar

Basta dar dois cliques no executável ou executar pelo terminal:

```powershell
.\sizescan_plus.exe
```

---

## 🔨 Como Recompilar

Basta rodar o script no terminal:
```cmd
build.bat
```

O script detecta o compilador MSVC (`cl.exe`), compila e linka as bibliotecas nativas do Windows (`user32`, `gdi32`, `shell32`, `comctl32`, `uxtheme`, `dwmapi`, `ole32`, `shlwapi`) incorporando o manifesto Common Controls v6 diretamente no executável.
