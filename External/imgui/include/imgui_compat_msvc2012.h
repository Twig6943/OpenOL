// MSVC 2012 (VC11) compatibility shims for Dear ImGui.
// Include this at the very top of each ImGui .cpp file.
#if defined(_MSC_VER) && _MSC_VER < 1800
// _ALLOW_KEYWORD_MACROS suppresses xkeycheck.h C1189: MSVC STL forbids macroizing C++ keywords,
// but MSVC 2012 does not support constexpr natively, so we must define it away.
// This define must appear before any STL header is included.
#ifndef _ALLOW_KEYWORD_MACROS
#define _ALLOW_KEYWORD_MACROS
#endif
#define constexpr /*constexpr*/
#ifndef nullptr
#define nullptr NULL
#endif
#endif
