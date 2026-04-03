// Minimal Scintilla constants needed for UELogViewer plugin
#pragma once
#include <windows.h>
#include <stddef.h>

typedef size_t    uptr_t;
typedef ptrdiff_t sptr_t;

// Style constants
#define STYLE_DEFAULT     32
#define STYLE_LINENUMBER  33
#define STYLE_BRACELIGHT  34
#define STYLE_BRACEBAD    35
#define STYLE_CONTROLCHAR 36
#define STYLE_INDENTGUIDE 37
#define STYLE_LASTPREDEFINED 39

// Scintilla messages
#define SCI_STYLESETFORE        2051
#define SCI_STYLESETBACK        2052
#define SCI_STYLESETBOLD        2053
#define SCI_STYLESETITALIC      2054
#define SCI_STYLESETSIZE        2055
#define SCI_STYLESETFONT        2056
#define SCI_STYLESETUNDERLINE   2059
#define SCI_STYLESETEOLFILLED   2057
#define SCI_STYLECLEARALL       2050
#define SCI_STYLESETCHARACTERSET 2066

#define SCI_SETLEXER            4001
#define SCI_GETLEXER            4002
#define SCI_SETLEXERLANGUAGE    4006
#define SCI_LOADLEXERLIBRARY    4007
#define SCI_COLOURISE           4003
#define SCI_SETPROPERTY         4004
#define SCI_SETKEYWORDS         4005

#define SCI_GETTEXT             2182
#define SCI_GETTEXTLENGTH       2183
#define SCI_GETLINECOUNT        2154
#define SCI_GETLINE             2153
#define SCI_GETLINELENGTH       2350
#define SCI_POSITIONFROMLINE    2167
#define SCI_GETLINEENDPOSITION  2136

#define SCI_STARTSTYLING        2032
#define SCI_SETSTYLING          2033
#define SCI_SETSTYLINGEX        2073

#define SCI_SETIDLESTYLING      2521

// Selection colors
#define SCI_SETSELFORE          2067
#define SCI_SETSELBACK          2068
#define SCI_SETSELFOREGROUND    2067  // alias
#define SCI_SETSELBACKGROUND    2068  // alias

// Caret color
#define SCI_SETCARETFORE        2069

// Additional line highlight
#define SCI_SETCARETLINEBACK    2094
#define SCI_SETCARETLINEVISIBLE 2096

#define SCLEX_CONTAINER         0
#define SCLEX_NULL              1

#define SC_MOD_INSERTTEXT       0x1
#define SC_MOD_DELETETEXT       0x2
#define SCN_MODIFIED            2008
#define SCN_UPDATEUI            2007
#define SCEN_CHANGE             768

// UpdateUI flags
#define SC_UPDATE_CONTENT       0x1
#define SC_UPDATE_SELECTION     0x2
#define SC_UPDATE_V_SCROLL      0x4
#define SC_UPDATE_H_SCROLL      0x8

// SCNotification structure (subset used by plugins)
struct SCNotification {
    NMHDR  nmhdr;
    int    position;
    int    ch;
    int    modifiers;
    int    modificationType;
    const char *text;
    int    length;
    int    linesAdded;
    int    message;
    uptr_t wParam;
    sptr_t lParam;
    int    line;
    int    foldLevelNow;
    int    foldLevelPrev;
    int    margin;
    int    listType;
    int    x;
    int    y;
    int    token;
    int    annotationLinesAdded;
    int    updated;
    int    listCompletionMethod;
};
