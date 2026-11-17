#ifndef PACKERT_HARDWARE__VISIBILITY_CONTROL_H_
#define PACKERT_HARDWARE__VISIBILITY_CONTROL_H_

// This logic was borrowed (then namespaced) from the examples on the gcc wiki:
//     https://gcc.gnu.org/wiki/Visibility

#if defined _WIN32 || defined __CYGWIN__
  #ifdef __GNUC__
    #define PACKERT_HARDWARE_EXPORT __attribute__ ((dllexport))
    #define PACKERT_HARDWARE_IMPORT __attribute__ ((dllimport))
  #else
    #define PACKERT_HARDWARE_EXPORT __declspec(dllexport)
    #define PACKERT_HARDWARE_IMPORT __declspec(dllimport)
  #endif
  #ifdef PACKERT_HARDWARE_BUILDING_LIBRARY
    #define PACKERT_HARDWARE_PUBLIC PACKERT_HARDWARE_EXPORT
  #else
    #define PACKERT_HARDWARE_PUBLIC PACKERT_HARDWARE_IMPORT
  #endif
  #define PACKERT_HARDWARE_PUBLIC_TYPE PACKERT_HARDWARE_PUBLIC
  #define PACKERT_HARDWARE_LOCAL
#else
  #define PACKERT_HARDWARE_EXPORT __attribute__ ((visibility("default")))
  #define PACKERT_HARDWARE_IMPORT
  #if __GNUC__ >= 4
    #define PACKERT_HARDWARE_PUBLIC __attribute__ ((visibility("default")))
    #define PACKERT_HARDWARE_LOCAL  __attribute__ ((visibility("hidden")))
  #else
    #define PACKERT_HARDWARE_PUBLIC
    #define PACKERT_HARDWARE_LOCAL
  #endif
  #define PACKERT_HARDWARE_PUBLIC_TYPE
#endif

#endif  // PACKERT_HARDWARE__VISIBILITY_CONTROL_H_
