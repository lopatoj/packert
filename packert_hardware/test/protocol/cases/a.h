#include "packert/protocol.h"

typedef struct __attribute__((packed)) {
  packert_header_t header;
  struct {
    struct {
      double position;
    } joint1;
  } joints;
} a_a_command_t;

typedef struct __attribute__((packed)) {
  packert_header_t header;
  struct {
    struct {
      double position;
    } joint1;
  } joints;
} a_a_state_t;
