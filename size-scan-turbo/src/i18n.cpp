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
        L"Nenhum arquivo encontrado correspondente aos critérios de busca ou filtro."
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
        L"No files matched your search or filter criteria."
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
        L"No se encontraron archivos que coincidan con la búsqueda o filtro."
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
