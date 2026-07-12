#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(BIOPIC_CORE_EXPORTS) || defined(BIOPIC_C_EXPORTS)
#define BIOPIC_API __declspec(dllexport)
#else
#define BIOPIC_API __declspec(dllimport)
#endif
#else
#define BIOPIC_API __attribute__((visibility("default")))
#endif
