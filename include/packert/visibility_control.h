#ifndef PACKERT__VISIBILITY_CONTROL_H_
#define PACKERT__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define PACKERT_EXPORT __attribute__ ((dllexport))
    #define PACKERT_IMPORT __attribute__ ((dllimport))
  #else
    #define PACKERT_EXPORT __declspec(dllexport)
    #define PACKERT_IMPORT __declspec(dllimport)
  #endif
  #ifdef PACKERT_BUILDING_LIBRARY
    #define PACKERT_PUBLIC PACKERT_EXPORT
  #else
    #define PACKERT_PUBLIC PACKERT_IMPORT
  #endif
  #define PACKERT_PUBLIC_TYPE PACKERT_PUBLIC
  #define PACKERT_LOCAL
#else
  #define PACKERT_EXPORT __attribute__ ((visibility("default")))
  #define PACKERT_IMPORT
  #if __GNUC__ >= 4
    #define PACKERT_PUBLIC __attribute__ ((visibility("default")))
    #define PACKERT_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define PACKERT_PUBLIC
    #define PACKERT_LOCAL
  #endif
  #define PACKERT_PUBLIC_TYPE
#endif

#endif  // PACKERT__VISIBILITY_CONTROL_H_
