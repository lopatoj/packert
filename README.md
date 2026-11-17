# packert

The focus of the project is to produce a library that enables the use of ros2_control with custom hardware, in the case that no hardware interface exists.

For example, a student may be working on a hobby robot where an affordable MCU manages motors and sensors. While it may be easy to get the robot moving and sensing with a simple Arduino script, connecting such a custom robot to the ROS 2 ecosystem presents some challenges.

Large companies bridge the gap by developing their own ros2_control hardware interface libraries, which implements the base classes defined by the framework. For example, Universal Robots provides a hardware interface within its ROS 2 driver package, which uses their custom RTDE communication protocol internally. This is a really nice setup, but it's not reasonable to expect a hobbyist to maintain this much.

A common alternative for applying ros2_control to a custom robot would be the combination of micro-ROS and the topic-based hardware interface. However, though the combination can be used for control, it brings DDS setup and an agent that aren’t typical of host-to-embedded links. Ideally, there is a solution targeting this ros2_control hardware interface implementation problem more directly.

By creating this library, I propose a workflow involving code generation derived from the command and state interfaces defined in a URDF `<ros2_control>` tag.

1. First, the user of this library would define these interfaces based on the hardware of their robot, e.g. joint commands for each motor, joint state for each encoder, sensor state for each sensor, gpio for anything else.
2. A CMake macro analagous to `generate_parameter_library` parses the description, and generates a framed protocol for communication between a host and an MCU. For example, generating a C packed struct. Ideally, code for sending and receiving on the firmware end is provided to the user.
3. This protocol could then be used when writing robot firmware, allowing the user to call methods for receiving and sending packets holding robot commands and state.
4. An implementation of a hardware interface supporting the generated protocol will then allow the robot firmware to communicate to ros2_control.

## Goals

- Provide a user-friendly API that can be used in C/C++ code targeting a general-purpose computer, bare-metal embedded system, or RTOS environment.
- Allow the substitution of the transport used for communication.
- Provide the same non-blocking, allocation-free, and deterministic guarantees required by the ros2_control framework.

## Decisions

- Targets: ROS 2 Jazzy, C++17, C99 for core library
- License: Apache-2.0

## Abstractions

These should all be swappable and implementable by user:

- Transport
- Watchdog Timer
- RTC Synchronization

## Open Questions

1. Should this library generate a new hardware interface for each protocol, or should this library provide a generic interface?

   a) Ideally, generic. A deterministic protocol generator should be able to be used by the interface at runtime.

2. How should changes to a robot discription be reflected in the changing protocol?

   a) Protocol hash is a first step, allowing a mismatch to be sussed out in a handshake.

3. How to allow for having a watchdog timer on the firmware end, while supporting multiple firmware environments?
4. How should timing work? Host and firmware wait for other to send before sending, or both fire at their own rates?
5. Should the firmware library target C or C++?

   a) Core should be C99, hardware interface in C++17, and C++ firmware wrappers around C core.

6. One-to-one between a `<ros2_control>` tag and an interface?

   a) If someone needs something else then I don't really care about them, given the stated user profile.

7. What is the plan for the lifecycle of the hardware interface implementation?

   a) For now:

   1. on_init receives HardwareInfo,
   2. on_configure computes hash and protocol at runtime
   3. on_activate spins up transport and attempts handshake

8. Should Host I/O be non_blocking or use a separate thread and lock free buffer? What libraries could be used for I/O, or should none be depended on?
9. How should transport implementations be swapped on the host?

   a) One way to do this is pluginlib

## Protocol

1. How should a protocol struct be organized?
2. How should a protocol struct be deterministic?
3. How should structs be namespaced?
4. How should URDF prefixes be handled? Ideally this shouldn't make a difference in a protocol or hash.
5. How should invalid characters in joint or sensor names behandled?

   a) Request that all such names in attributes be in snake_case, as most URDFs will comply. Also check for C/C++ keywords.

### Rules

0. Any attribute in the provided URDF file used as a name must be written in snake_case.

1. For each protocol, there exists a command and state struct, named first for the robot name derived from the `<robot>` tag, and then for the name of the `<ros2_control>` tag.

```c
[robot_name]_[system_name]_command_t and [robot_name]_[system_name]_state_t
```

2. Following the header, there may be 3 inline defined structs in the order of joints, sensors, and gpio. These structs only exist if there is at least one corresponding tag in the `<ros2_control>` tag.

```c
typedef struct __attribute__((packed)) {
  struct {
    // named joints go here
  } joint;
  struct {
    // named sensors go here
  } sensor;
  struct {
    // named gpios go here
  } gpio;
} robot_command_payload_t;
```

3. Each member of each of these subgroups are sorted ASCII lexicographically. This is deterministic, as two members of the same tag group may not share a name.

```c
typedef struct __attribute__((packed)) {
  struct {
    struct {
      // joint a's interfaces go here
    } a;
    struct {
      // joint b's interfaces go here
    } b;
  } joint;
} robot_command_data_t;
```

4. Additionally, within a specific joint/sensor/gpio, the interface names are also sorted ASCII lexicographically.

```c
typedef struct __attribute__((packed)) {
  struct {
    struct {
      float64_t effort;
      float64_t position;
      float64_t velocity;
    } joint1;
  } joint;
} robot_command_data_t;
```

5. For the first version, we will require that the user specifies the `data_type` attribute on interfaces if their embedded hardware is a 32bit architecture without satisfactory support for `float64`. For all other types, there will be a direct conversion between the URDF type and the C struct type. Down the line, it may be advantageous to have a interface-level parameter for automatically converting all interfaces without the specified data type to within 32bit. The hardware interface would then convert between 32bit over the the wire and the 64bit `float64_t` default stored in each interface.

6. The protocol header will be defined as the following struct.

```c
typedef enum {
  HANDSHAKE = 0,
  COMMAND,
  STATE,
} packert_type_t;

typedef struct __attribute__((packed)) {
  uint8_t version;
  packert_type_t type; // uint8_t
  uint16_t flags;
  uint16_t length;
  uint16_t sequence;
} packert_header_t;

typedef struct __attribute__((packed)) {
  packert_header_t header;
  uint32_t hash;
  uint16_t packert_version;
  uint16_t watchdog;
} packert_handshake_t;
```

7. Command, state, and handshake messages will have the following additional fields.

```c
typedef struct __attribute__((packed)) {
  packert_header_t header;
  uint32_t host_tx_us;
  robot_command_data_t data;
} robot_command_t;

typedef struct __attribute__((packed)) {
  packert_header_t header;
  uint32_t hash;
  uint16_t host_rx_seq;
  uint32_t host_rx_us;
  uint32_t device_tx_us;
  packert_status_t status;
  robot_state_data_t data;
} robot_state_t;
```

8. All fields will have little-endian byte ordering.

9. Types map directly in accordance to the float64_as_float32 and int64_as_int32 flags.

## API Planning

### Transport

Ideally, transport implementations are provided for both host and firmware targets, such as linux serial, or
Zephyr RTOS. However, this library should still be open to a user-defined alternative. Note: `std::span` is subject to change.

```cpp

class Transport {
  virtual ~Transport = default;

  virtual bool send(std::span<const std::byte> buffer) = 0;
  virtual bool receive(std::span<std::byte> buffer) = 0
}

```
