#ifndef I18N_H
#define I18N_H

#include <windows.h>
#include <string>

enum AppLanguage {
    LANG_PT = 0, // Português
    LANG_EN = 1, // English
    LANG_ES = 2, // Español
    LANG_COUNT = 3
};

enum StringId {
    STR_APP_TITLE,
    STR_APP_SUBTITLE_PREFIX,
    STR_THEME_LIGHT,
    STR_THEME_DARK,
    STR_LANG_LABEL,
    STR_DRIVE_PREFIX,
    STR_CHOOSE_FOLDER,
    STR_RESCAN,
    STR_MODE_ON_DISK,
    STR_MODE_REAL_SIZE,
    STR_SUBFOLDERS_HIDDEN,
    STR_SUBFOLDERS_VISIBLE,

    // Abas de navegação
    STR_TAB_OVERVIEW,
    STR_TAB_TOP_FILES,
    STR_TAB_TOP_FOLDERS,
    STR_TAB_ALL_FILES,

    // Barra de progresso / scanning
    STR_SCANNING_PREFIX,
    STR_FILES_LABEL,
    STR_STOP,

    // Aba Visão Geral
    STR_DISK_CAPACITY,
    STR_USED_OF,
    STR_FREE_OF,
    STR_CLUSTER_LABEL,
    STR_STORAGE_DISTRIBUTION,
    STR_TOTAL_ANALYZED,
    STR_CLICK_FILTER_HINT,
    STR_SUB_ARQ_LABEL,

    // Categorias
    STR_CAT_VIDEOS,
    STR_CAT_IMAGES,
    STR_CAT_AUDIO,
    STR_CAT_ARCHIVES,
    STR_CAT_DOCS,
    STR_CAT_EXEC,
    STR_CAT_CODE,
    STR_CAT_OTHER,

    // Aba Top Arquivos
    STR_COL_RANK,
    STR_COL_TYPE,
    STR_COL_FILENAME,
    STR_COL_SIZE,
    STR_COL_ACTIONS,
    STR_BTN_OPEN_FOLDER,

    // Aba Top Pastas
    STR_COL_FOLDER_PATH,
    STR_COL_COUNT_FILES,
    STR_COL_COUNT_SUBS,

    // Aba Todos os Arquivos
    STR_FILTER_SIZE_TITLE,
    STR_FILTER_ALL,
    STR_FILTER_CAT_TITLE,
    STR_FILTER_ALL_CATS,
    STR_BTN_CLEAR_FILTERS,
    STR_SEARCH_CUE,
    STR_RESULTS_TITLE,
    STR_EMPTY_SEARCH,

    STR_TOTAL_STRINGS
};

class I18nManager {
public:
    I18nManager();

    AppLanguage GetLanguage() const { return m_lang; }
    void SetLanguage(AppLanguage lang);
    void NextLanguage();

    const wchar_t* Get(StringId id) const;
    const wchar_t* GetCategoryName(int catIndex) const;
    const wchar_t* GetLangButtonLabel() const;
    const wchar_t* GetLangCode() const;

private:
    AppLanguage m_lang;
};

extern I18nManager g_i18n;

#define TR(id) g_i18n.Get(id)

#endif // I18N_H
