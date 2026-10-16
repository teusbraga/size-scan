#include "i18n.h"

I18nManager g_i18n;

static const wchar_t* g_translations[LANG_COUNT][STR_TOTAL_STRINGS] = {
    // -------------------------------------------------------------
    // LANG_PT (Português do Brasil)
    // -------------------------------------------------------------
    {
        L"SIZE-SCAN PLUS",
        L"Scanner Ultraleve e Ultrarrápido de Disco  |  Pasta: ",
        L"☀ Modo Claro",
        L"🌙 Modo Escuro",
        L"🌐 PT",
        L"Disco ",
        L"Escolher Pasta...",
        L"↻ Re-escanear",
        L"Modo: Em Disco (Clusters)",
        L"Modo: Tamanho Real (Bytes)",
        L"Subpastas: Ocultas (Filtro Ativo)",
        L"Subpastas: Visíveis (Todas)",

        // Abas
        L"Visão Geral (Gráficos)",
        L"Top Arquivos Grandes",
        L"Pastas Mais Pesadas",
        L"Todos os Arquivos & Filtros",

        // Barra de progresso
        L"Escaneando: ",
        L"arquivos",
        L"Parar",

        // Visão Geral
        L"CAPACIDADE DO DISCO",
        L" Usados (",
        L" Livres de ",
        L"  [Cluster: ",
        L"DISTRIBUIÇÃO DE ARMAZENAMENTO",
        L"Total Analisado",
        L"Clique em qualquer categoria ao lado para filtrar",
        L" arquivos",

        // Categorias
        L"Vídeos",
        L"Imagens",
        L"Áudios",
        L"Compactados",
        L"Documentos",
        L"Executáveis",
        L"Código",
        L"Outros",

        // Top Arquivos
        L"#",
        L"TIPO",
        L"NOME DO ARQUIVO",
        L"TAMANHO",
        L"AÇÕES",
        L"Abrir Pasta",

        // Top Pastas
        L"CAMINHO DA PASTA",
        L" arq / ",
        L" sub",

        // Todos os Arquivos
        L"FILTRAR POR TAMANHO:",
        L"Todos",
        L"CATEGORIA:",
        L"Todas as Categorias",
        L"Limpar Filtros",
        L"Digite para filtrar arquivos por nome ou extensão (ex: .mp4, .zip, setup)...",
        L"RESULTADOS (",
        L"Nenhum arquivo encontrado correspondente aos critérios de busca ou filtro.",

        // Menu Bar
        L"&Arquivo",
        L"&Exibir",
        L"A&parência",
        L"A&juda",
        L"&Sair\tAlt+F4",
        L"&Idioma",
        L"Fonte + (Aumentar)\tCtrl +",
        L"Fonte - (Diminuir)\tCtrl -",
        L"Tamanho Normal (100%)\tCtrl 0",
        L"Como Usar / Guia Rápido...\tF1",
        L"Sobre o Size-Scan Plus...",

        // Diálogos Nativos
        L"Sobre o Size-Scan Plus",
        L"Size-Scan Plus v2.0 (x64 Nativo)\n\n"
        L"Scanner Ultraleve e Ultrarrápido de Armazenamento em Disco.\n"
        L"Desenvolvido com C++ e Win32 puro.\n\n"
        L"• Varredura Multithread de Alta Performance com E/S Otimizada\n"
        L"• Cálculo Real de Espaço em Disco (Clusters e Arquivos Esparsos)\n"
        L"• Consumo Ultrabaixo de Memória RAM com String Pooling\n"
        L"• Compatível com Modo Claro e Escuro do Windows 11\n\n"
        L"Criado com dedicação para máxima velocidade e leveza.",
        L"Guia Rápido do Size-Scan Plus",
        L"GUIA DE RECURSOS E ATALHOS:\n\n"
        L"1. Tamanho Real vs. Em Disco (Clusters):\n"
        L"   O 'Tamanho Real' soma os bytes lógicos dos arquivos. O modo 'Em Disco' calcula os blocos físicos reais alocados pelo sistema de arquivos NTFS/FAT, incluindo arquivos esparsos (ex: imagens de disco virtual).\n\n"
        L"2. Filtro de Subpastas Redundantes:\n"
        L"   Quando ativado, esconde pastas filhas se a pasta pai já está no topo, evitando duplicação visual nas pastas mais pesadas.\n\n"
        L"3. Atalhos de Acessibilidade:\n"
        L"   • Ctrl +  : Aumentar tamanho das fontes\n"
        L"   • Ctrl -  : Diminuir tamanho das fontes\n"
        L"   • Ctrl 0  : Redefinir fonte para 100%\n"
        L"   • F1      : Abrir este guia de ajuda"
    },

    // -------------------------------------------------------------
    // LANG_EN (English)
    // -------------------------------------------------------------
    {
        L"SIZE-SCAN PLUS",
        L"Ultralight & Ultra-fast Disk Scanner  |  Folder: ",
        L"☀ Light Mode",
        L"🌙 Dark Mode",
        L"🌐 EN",
        L"Drive ",
        L"Choose Folder...",
        L"↻ Rescan",
        L"Mode: On Disk (Clusters)",
        L"Mode: Real Size (Bytes)",
        L"Subfolders: Hidden (Filter On)",
        L"Subfolders: Visible (All)",

        // Abas
        L"Overview (Charts)",
        L"Top Large Files",
        L"Heaviest Folders",
        L"All Files & Filters",

        // Barra de progresso
        L"Scanning: ",
        L"files",
        L"Stop",

        // Visão Geral
        L"DISK CAPACITY",
        L" Used (",
        L" Free of ",
        L"  [Cluster: ",
        L"STORAGE DISTRIBUTION",
        L"Total Analyzed",
        L"Click any category on the right to filter",
        L" files",

        // Categorias
        L"Videos",
        L"Images",
        L"Audio",
        L"Archives",
        L"Documents",
        L"Executables",
        L"Source Code",
        L"Others",

        // Top Arquivos
        L"#",
        L"TYPE",
        L"FILE NAME",
        L"SIZE",
        L"ACTIONS",
        L"Open Folder",

        // Top Pastas
        L"FOLDER PATH",
        L" files / ",
        L" sub",

        // Todos os Arquivos
        L"FILTER BY SIZE:",
        L"All",
        L"CATEGORY:",
        L"All Categories",
        L"Clear Filters",
        L"Type to filter files by name or extension (e.g. .mp4, .zip, setup)...",
        L"RESULTS (",
        L"No files matched your search or filter criteria.",

        // Menu Bar
        L"&File",
        L"&View",
        L"&Appearance",
        L"&Help",
        L"&Exit\tAlt+F4",
        L"&Language",
        L"Font + (Increase)\tCtrl +",
        L"Font - (Decrease)\tCtrl -",
        L"Reset Font Size (100%)\tCtrl 0",
        L"How to Use / Quick Guide...\tF1",
        L"About Size-Scan Plus...",

        // Diálogos Nativos
        L"About Size-Scan Plus",
        L"Size-Scan Plus v2.0 (x64 Native)\n\n"
        L"Ultralight & Ultra-fast Disk Storage Scanner.\n"
        L"Built with pure modern C++ and Win32.\n\n"
        L"• High-Performance Multithreaded Scanning with Optimized I/O\n"
        L"• Accurate Real-Size and On-Disk (Cluster & Sparse File) Tracking\n"
        L"• Ultralow RAM Footprint with Block String Pooling\n"
        L"• Native Windows 11 Dark and Light Mode Support\n\n"
        L"Engineered for maximum speed and minimal memory footprint.",
        L"Size-Scan Plus Quick Guide",
        L"QUICK GUIDE & SHORTCUTS:\n\n"
        L"1. Logical Size vs. Size on Disk:\n"
        L"   'Logical Size' counts actual file bytes. 'Size on Disk' computes the physical clusters allocated on NTFS/FAT, including sparse virtual disks.\n\n"
        L"2. Redundant Subfolders Filter:\n"
        L"   When active, hides descendant subfolders if their parent directory is already listed in top rankings, eliminating visual duplication.\n\n"
        L"3. Accessibility Shortcuts:\n"
        L"   • Ctrl +  : Increase font size\n"
        L"   • Ctrl -  : Decrease font size\n"
        L"   • Ctrl 0  : Reset font size to 100%\n"
        L"   • F1      : Open this guide"
    },

    // -------------------------------------------------------------
    // LANG_ES (Español)
    // -------------------------------------------------------------
    {
        L"SIZE-SCAN PLUS",
        L"Escáner Ultraligero y Ultrarrápido de Disco  |  Carpeta: ",
        L"☀ Modo Claro",
        L"🌙 Modo Oscuro",
        L"🌐 ES",
        L"Disco ",
        L"Elegir Carpeta...",
        L"↻ Re-escanear",
        L"Modo: En Disco (Clusters)",
        L"Modo: Tamaño Real (Bytes)",
        L"Subcarpetas: Ocultas (Filtro Activo)",
        L"Subcarpetas: Visibles (Todas)",

        // Abas
        L"Visión General (Gráficos)",
        L"Archivos Más Grandes",
        L"Carpetas Más Pesadas",
        L"Todos los Archivos y Filtros",

        // Barra de progresso
        L"Escaneando: ",
        L"archivos",
        L"Detener",

        // Visão Geral
        L"CAPACIDAD DEL DISCO",
        L" Usados (",
        L" Libres de ",
        L"  [Cluster: ",
        L"DISTRIBUCIÓN DE ALMACENAMIENTO",
        L"Total Analizado",
        L"Haga clic en cualquier categoría para filtrar",
        L" archivos",

        // Categorias
        L"Vídeos",
        L"Imágenes",
        L"Audios",
        L"Comprimidos",
        L"Documentos",
        L"Ejecutables",
        L"Código",
        L"Otros",

        // Top Arquivos
        L"#",
        L"TIPO",
        L"NOMBRE DEL ARCHIVO",
        L"TAMAÑO",
        L"ACCIONES",
        L"Abrir Carpeta",

        // Top Pastas
        L"RUTA DE LA CARPETA",
        L" arch / ",
        L" sub",

        // Todos os Arquivos
        L"FILTRAR POR TAMAÑO:",
        L"Todos",
        L"CATEGORÍA:",
        L"Todas las Categorías",
        L"Limpiar Filtros",
        L"Escriba para filtrar archivos por nombre o extensión (ej: .mp4, .zip, setup)...",
        L"RESULTADOS (",
        L"No se encontraron archivos que coincidan con la búsqueda o filtro.",

        // Menu Bar
        L"&Archivo",
        L"&Ver",
        L"A&pariencia",
        L"A&yuda",
        L"&Salir\tAlt+F4",
        L"&Idioma",
        L"Fuente + (Aumentar)\tCtrl +",
        L"Fuente - (Disminuir)\tCtrl -",
        L"Tamaño Normal (100%)\tCtrl 0",
        L"Cómo Usar / Guía Rápida...\tF1",
        L"Acerca de Size-Scan Plus...",

        // Diálogos Nativos
        L"Acerca de Size-Scan Plus",
        L"Size-Scan Plus v2.0 (x64 Nativo)\n\n"
        L"Escáner Ultraligero y Ultrarrápido de Almacenamiento en Disco.\n"
        L"Desarrollado con C++ puro y Win32.\n\n"
        L"• Escaneo Multihilo de Alto Rendimiento con E/S Optimizada\n"
        L"• Cálculo Preciso de Espacio en Disco (Clusters y Archivos Dispersos)\n"
        L"• Consumo Mínimo de Memoria RAM con String Pooling\n"
        L"• Compatible con Modo Claro y Oscuro de Windows 11\n\n"
        L"Diseñado para máxima velocidad y eficiencia.",
        L"Guía Rápida de Size-Scan Plus",
        L"GUÍA DE CARACTERÍSTICAS Y ATAJOS:\n\n"
        L"1. Tamaño Real vs. En Disco (Clusters):\n"
        L"   'Tamaño Real' suma los bytes lógicos. 'En Disco' calcula los clusters físicos asignados por el sistema de archivos NTFS/FAT, incluyendo archivos dispersos.\n\n"
        L"2. Filtro de Subcarpetas Redundantes:\n"
        L"   Oculta subcarpetas hijas si la carpeta superior ya aparece en la lista.\n\n"
        L"3. Atajos de Accesibilidad:\n"
        L"   • Ctrl +  : Aumentar tamaño de fuente\n"
        L"   • Ctrl -  : Disminuir tamaño de fuente\n"
        L"   • Ctrl 0  : Restablecer tamaño a 100%\n"
        L"   • F1      : Abrir esta guía"
    }
};

I18nManager::I18nManager() : m_lang(LANG_PT) {}

void I18nManager::SetLanguage(AppLanguage lang) {
    if (lang >= 0 && lang < LANG_COUNT) {
        m_lang = lang;
    }
}

void I18nManager::NextLanguage() {
    int next = (int)m_lang + 1;
    if (next >= LANG_COUNT) next = 0;
    m_lang = (AppLanguage)next;
}

const wchar_t* I18nManager::Get(StringId id) const {
    if (id >= 0 && id < STR_TOTAL_STRINGS) {
        return g_translations[m_lang][id];
    }
    return L"";
}

const wchar_t* I18nManager::GetCategoryName(int catIndex) const {
    if (catIndex >= 0 && catIndex < 8) {
        return g_translations[m_lang][STR_CAT_VIDEOS + catIndex];
    }
    return L"";
}

const wchar_t* I18nManager::GetLangButtonLabel() const {
    switch (m_lang) {
        case LANG_PT: return L"PT";
        case LANG_EN: return L"EN";
        case LANG_ES: return L"ES";
        default: return L"PT";
    }
}

const wchar_t* I18nManager::GetLangCode() const {
    switch (m_lang) {
        case LANG_PT: return L"PT";
        case LANG_EN: return L"EN";
        case LANG_ES: return L"ES";
        default: return L"PT";
    }
}
