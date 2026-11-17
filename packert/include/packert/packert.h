#ifndef PACKERT_H
#define PACKERT_H

#if defined(__GNUC__) || defined(__clang__)
#define PACKERT_PACKED __attribute__((packed))
#else
#error "packert: unsupported compiler (need packed struct support)"
#endif

#ifdef __cplusplus
#define PACKERT_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define PACKERT_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

typedef enum {
  PACKERT_STATUS_DISABLED = 0,
  PACKERT_STATUS_ENABLED,
  PACKERT_STATUS_FAULT = 2
} packert_status_t;

typedef enum {
  HANDSHAKE = 0,
  COMMAND,
  STATE,
} packert_type_t;

typedef struct __attribute__((packed)) {
  packert_status_t status;
} packert_header_t;

typedef struct __attribute__((packed)) {
  packert_header_t header;
} handshake_host_t;

typedef struct __attribute__((packed)) {
  packert_header_t header;
} packert_handshake_t;

#endif  // PACKERT_H
