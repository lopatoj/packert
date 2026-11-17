#include "packert/protocol.h"

typedef struct __attribute__((packed)) {
  packert_header_t header;
  struct {
    struct {
      double position;
    } a;
    struct {
      double position;
    } b;
  } joints;
} b_b_command_t;

typedef struct __attribute__((packed)) {
  packert_header_t header;
  struct {
    struct {
      double position;
    } a;
    struct {
      double position;
    } b;
  } joints;
} b_b_state_t;
