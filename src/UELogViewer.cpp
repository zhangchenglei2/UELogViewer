/**
 * UELogViewer - Notepad++ Plugin
 * Syntax highlighting for Unreal Engine log files
 *
 * Supports coloring by log level:
 *   Fatal   - bright red background
 *   Error   - red
 *   Warning - orange/yellow
 *   Display - cyan
 *   Log     - default
 *   Verbose - gray
 *   VeryVerbose - dark gray
 *
 * Build: See UELogViewer.vcxproj or README.md
 */

#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <regex>
#include "ScintillaDefs.h"
#include "PluginInterface.h"

// ---------------------------------------------------------------------------
// Plugin metadata
// ---------------------------------------------------------------------------
static const TCHAR PLUGIN_NAME[] = TEXT("UELogViewer");
static NppData g_nppData;
static HINSTANCE g_hModule = nullptr;

// ---------------------------------------------------------------------------
// Style IDs (Scintilla user-defined styles start at 0 for SCLEX_CONTAINER)
// We use styles 0-9 for our log levels.
// ---------------------------------------------------------------------------
enum UELogStyle : int {
    STYLE_UE_DEFAULT      = 0,
    STYLE_UE_FATAL        = 1,
    STYLE_UE_ERROR        = 2,
    STYLE_UE_WARNING      = 3,
    STYLE_UE_DISPLAY      = 4,
    STYLE_UE_LOG          = 5,
    STYLE_UE_VERBOSE      = 6,
    STYLE_UE_VERYVERBOSE  = 7,
    STYLE_UE_TIMESTAMP    = 8,
    STYLE_UE_CATEGORY     = 9,
};

// RGB helper (Scintilla uses BGR internally but SendMessage takes 0xBBGGRR)
#define RGB_TO_SCINTILLA(r,g,b) ((LPARAM)(((b)<<16)|((g)<<8)|(r)))

struct StyleDef {
    int         id;
    COLORREF    fore;       // 0xBBGGRR for Scintilla
    COLORREF    back;       // background, 0 = transparent (use default)
    bool        bold;
    bool        eolFilled;  // fill background to end of line
};

static const StyleDef k_styles[] = {
    // id                    fore (BGR)                  back                        bold   eolFill
    { STYLE_UE_DEFAULT,      RGB_TO_SCINTILLA(220,220,220), 0,                        false, false },
    { STYLE_UE_FATAL,        RGB_TO_SCINTILLA(255,255,255), RGB_TO_SCINTILLA(180,0,0),  true,  true  }, // white on dark red
    { STYLE_UE_ERROR,        RGB_TO_SCINTILLA(255, 80, 80), 0,                        true,  false },
    { STYLE_UE_WARNING,      RGB_TO_SCINTILLA(255,200, 50), 0,                        false, false },
    { STYLE_UE_DISPLAY,      RGB_TO_SCINTILLA( 80,200,255), 0,                        false, false },
    { STYLE_UE_LOG,          RGB_TO_SCINTILLA(220,220,220), 0,                        false, false },
    { STYLE_UE_VERBOSE,      RGB_TO_SCINTILLA(140,140,140), 0,                        false, false },
    { STYLE_UE_VERYVERBOSE,  RGB_TO_SCINTILLA( 90, 90, 90), 0,                        false, false },
    { STYLE_UE_TIMESTAMP,    RGB_TO_SCINTILLA( 80,160, 80), 0,                        false, false },
    { STYLE_UE_CATEGORY,     RGB_TO_SCINTILLA(180,130,255), 0,                        false, false },
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static HWND getCurrentScintilla() {
    int which = 0;
    ::SendMessage(g_nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    return which == 0 ? g_nppData._scintillaMainHandle : g_nppData._scintillaSecondHandle;
}

static LRESULT sciSend(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    return ::SendMessage(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
// Apply styles to Scintilla
// ---------------------------------------------------------------------------
static void applyStyles(HWND hSci) {
    // Set lexer to CONTAINER so we drive all styling
    sciSend(hSci, SCI_SETLEXER, SCLEX_CONTAINER, 0);

    // Dark background
    COLORREF darkBg = RGB_TO_SCINTILLA(30, 30, 30);

    // Clear all styles to our dark background first
    sciSend(hSci, SCI_STYLESETBACK, STYLE_DEFAULT, darkBg);
    sciSend(hSci, SCI_STYLECLEARALL, 0, 0);

    // Apply each defined style
    for (const auto& s : k_styles) {
        sciSend(hSci, SCI_STYLESETFORE, s.id, s.fore);
        if (s.back != 0)
            sciSend(hSci, SCI_STYLESETBACK, s.id, s.back);
        else
            sciSend(hSci, SCI_STYLESETBACK, s.id, darkBg);
        sciSend(hSci, SCI_STYLESETBOLD,      s.id, s.bold ? 1 : 0);
        sciSend(hSci, SCI_STYLESETEOLFILLED,  s.id, s.eolFilled ? 1 : 0);
    }
}

// ---------------------------------------------------------------------------
// Parse a single line and return its style
//
// UE log line formats:
//   [2024.03.15-10.22.31:123][  0]LogCategory: Verbosity: message
//   [2024.03.15-10.22.31:123][  0]LogCategory: message
//   LogCategory: Verbosity: message   (no timestamp)
//   LogCategory: message
// ---------------------------------------------------------------------------
struct LineSegment {
    int start;  // byte offset within line
    int length;
    int style;
};

static std::vector<LineSegment> parseLine(const std::string& line) {
    std::vector<LineSegment> segs;
    int len = (int)line.size();
    if (len == 0) return segs;

    int pos = 0;

    // --- Timestamp block: [YYYY.MM.DD-HH.MM.SS:mmm][  N] ---
    if (pos < len && line[pos] == '[') {
        int end = (int)line.find(']', pos);
        if (end != (int)std::string::npos) {
            int end2 = (int)line.find(']', end + 1);
            if (end2 != (int)std::string::npos) {
                segs.push_back({ pos, end2 + 1 - pos, STYLE_UE_TIMESTAMP });
                pos = end2 + 1;
            }
        }
    }

    // --- Category: LogXxx ---
    int colonPos = (int)line.find(':', pos);
    if (colonPos != (int)std::string::npos) {
        // Category is the word before the first colon
        std::string category = line.substr(pos, colonPos - pos);
        bool looksLikeCategory = !category.empty() &&
            (category.find(' ') == std::string::npos) &&
            (isupper((unsigned char)category[0]) || category[0] == '_');

        if (looksLikeCategory) {
            segs.push_back({ pos, colonPos - pos, STYLE_UE_CATEGORY });
            segs.push_back({ colonPos, 1, STYLE_UE_DEFAULT }); // colon
            pos = colonPos + 1;

            // Skip space
            if (pos < len && line[pos] == ' ') pos++;

            // --- Verbosity level ---
            // Check for known verbosity keywords before the next colon
            int nextColon = (int)line.find(':', pos);
            if (nextColon != (int)std::string::npos) {
                std::string verbStr = line.substr(pos, nextColon - pos);

                // Trim leading/trailing spaces
                size_t vs = verbStr.find_first_not_of(' ');
                size_t ve = verbStr.find_last_not_of(' ');
                if (vs != std::string::npos)
                    verbStr = verbStr.substr(vs, ve - vs + 1);

                int verbStyle = -1;
                if      (verbStr == "Fatal")       verbStyle = STYLE_UE_FATAL;
                else if (verbStr == "Error")        verbStyle = STYLE_UE_ERROR;
                else if (verbStr == "Warning")      verbStyle = STYLE_UE_WARNING;
                else if (verbStr == "Display")      verbStyle = STYLE_UE_DISPLAY;
                else if (verbStr == "Log")          verbStyle = STYLE_UE_LOG;
                else if (verbStr == "Verbose")      verbStyle = STYLE_UE_VERBOSE;
                else if (verbStr == "VeryVerbose")  verbStyle = STYLE_UE_VERYVERBOSE;

                if (verbStyle >= 0) {
                    segs.push_back({ pos, nextColon + 1 - pos, verbStyle });
                    pos = nextColon + 1;
                    // Remainder of line gets same style
                    if (pos < len)
                        segs.push_back({ pos, len - pos, verbStyle });
                    return segs;
                }
            }

            // No explicit verbosity — try to detect from message content
            std::string rest = line.substr(pos);
            int msgStyle = STYLE_UE_LOG;
            // Some logs omit verbosity; use heuristics
            if (rest.find("Error")   != std::string::npos && rest.size() < 20) msgStyle = STYLE_UE_ERROR;
            if (rest.find("Warning") != std::string::npos && rest.size() < 20) msgStyle = STYLE_UE_WARNING;

            if (pos < len)
                segs.push_back({ pos, len - pos, msgStyle });
            return segs;
        }
    }

    // Fallback: whole line default
    segs.push_back({ 0, len, STYLE_UE_DEFAULT });
    return segs;
}

// ---------------------------------------------------------------------------
// Re-style the entire document
// ---------------------------------------------------------------------------
static void colouriseDocument(HWND hSci) {
    int totalLen = (int)sciSend(hSci, SCI_GETTEXTLENGTH, 0, 0);
    if (totalLen <= 0) return;

    // Grab all text
    std::string text(totalLen + 1, '\0');
    sciSend(hSci, SCI_GETTEXT, totalLen + 1, (LPARAM)text.data());
    text.resize(totalLen);

    sciSend(hSci, SCI_STARTSTYLING, 0, 0);

    int lineCount = (int)sciSend(hSci, SCI_GETLINECOUNT, 0, 0);
    for (int i = 0; i < lineCount; i++) {
        int lineStart  = (int)sciSend(hSci, SCI_POSITIONFROMLINE, i, 0);
        int lineEnd    = (int)sciSend(hSci, SCI_GETLINEENDPOSITION, i, 0);
        int lineLen    = lineEnd - lineStart;
        if (lineLen <= 0) {
            // empty line — style 1 byte as default if we're not at end
            if (lineStart < totalLen)
                sciSend(hSci, SCI_SETSTYLING, 1, STYLE_UE_DEFAULT);
            continue;
        }

        std::string lineText = text.substr(lineStart, lineLen);
        auto segs = parseLine(lineText);

        // Reconstruct flat style array for this line
        std::vector<char> styles(lineLen, (char)STYLE_UE_DEFAULT);
        for (auto& seg : segs) {
            int s = seg.start;
            int e = s + seg.length;
            if (e > lineLen) e = lineLen;
            for (int j = s; j < e; j++)
                styles[j] = (char)seg.style;
        }

        sciSend(hSci, SCI_STARTSTYLING, lineStart, 0);
        sciSend(hSci, SCI_SETSTYLINGEX, lineLen, (LPARAM)styles.data());
    }
}

// ---------------------------------------------------------------------------
// Activate: apply styles + colourise current document
// ---------------------------------------------------------------------------
static void activateHighlighting() {
    HWND hSci = getCurrentScintilla();
    applyStyles(hSci);
    colouriseDocument(hSci);
    ::MessageBox(g_nppData._nppHandle,
        TEXT("UE Log highlighting applied!\n\nColors:\n  Fatal   - white on red\n  Error   - red\n  Warning - yellow\n  Display - cyan\n  Log     - light gray\n  Verbose - gray\n  VeryVerbose - dark gray"),
        TEXT("UELogViewer"), MB_OK | MB_ICONINFORMATION);
}

static void resetHighlighting() {
    HWND hSci = getCurrentScintilla();
    sciSend(hSci, SCI_SETLEXER, SCLEX_NULL, 0);
    sciSend(hSci, SCI_STYLECLEARALL, 0, 0);
}

static void showAbout() {
    ::MessageBox(g_nppData._nppHandle,
        TEXT("UELogViewer v1.0\n\nSyntax highlighting for Unreal Engine log files.\n\nSupports: Fatal / Error / Warning / Display / Log / Verbose / VeryVerbose"),
        TEXT("About UELogViewer"), MB_OK | MB_ICONINFORMATION);
}

// ---------------------------------------------------------------------------
// Plugin menu items
// ---------------------------------------------------------------------------
static FuncItem g_funcItems[] = {
    { TEXT("Apply UE Log Highlighting"),  activateHighlighting, 0, false, nullptr },
    { TEXT("Reset Highlighting"),         resetHighlighting,    0, false, nullptr },
    { TEXT("---"),                        nullptr,              0, false, nullptr },
    { TEXT("About"),                      showAbout,            0, false, nullptr },
};
static const int NB_FUNC = sizeof(g_funcItems) / sizeof(g_funcItems[0]);

// ---------------------------------------------------------------------------
// Exported plugin functions (Notepad++ API)
// ---------------------------------------------------------------------------
extern "C" {

__declspec(dllexport) void setInfo(NppData data) {
    g_nppData = data;
}

__declspec(dllexport) const TCHAR * getName() {
    return PLUGIN_NAME;
}

__declspec(dllexport) FuncItem * getFuncsArray(int *nbF) {
    *nbF = NB_FUNC;
    return g_funcItems;
}

__declspec(dllexport) void beNotified(SCNotification *notification) {
    // Auto re-highlight on SCN_UPDATEUI (content changed)
    // Only if lexer is already set to CONTAINER (we activated it)
    if (notification->nmhdr.code == SCN_UPDATEUI) {
        HWND hSci = (notification->nmhdr.hwndFrom == g_nppData._scintillaMainHandle)
                    ? g_nppData._scintillaMainHandle
                    : g_nppData._scintillaSecondHandle;
        int lexer = (int)sciSend(hSci, SCI_GETLEXER, 0, 0);
        if (lexer == SCLEX_CONTAINER) {
            colouriseDocument(hSci);
        }
    }
}

__declspec(dllexport) LRESULT messageProc(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/) {
    return TRUE;
}

#ifdef UNICODE
__declspec(dllexport) BOOL isUnicode() { return TRUE; }
#endif

} // extern "C"

// ---------------------------------------------------------------------------
// DLL entry point
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD reason, LPVOID /*reserved*/) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        ::DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
