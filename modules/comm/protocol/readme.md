# Protocol

Shared wire contract between Safety Mesh modules: message schemas and the
codec. No sockets, no application logic.

## Messages

| Schema | Direction | Transport |
|---|---|---|
| `proto/camera_frame.proto` | cam_node → manager | UDP, one `CameraFrame` per datagram |

## Datagram layout

```
[ protobuf payload ][ CRC-32 of payload, 4 bytes big-endian ]
```

- Default cap at 1400 bytes (`comm::protocol::kMaxDatagramSize`) to avoid IP fragmentation. check MTU (maximum transmission unit) for 
increased Datagram size
- `decode` checks length, CRC, parse, then `protocol_version`. Drop anything
  that fails.
- Sequence and timestamp checks are the receiver's job. Missing or stale
  frames mean unsafe.

## Protobuf

Protobuf is a binary serialization format. You define messages in a `.proto`
file and `protoc` generates C++ classes for them. Only field numbers and
values go on the wire, not names.

**Pros**
- One schema shared by sender and receiver; no hand-written parsing.
- Small messages.
- Fields can be added without breaking older nodes.
- Multi-Language utilization (C++, Rust, Python, etc...)

**Cons**
- Extra build dependency and code generation.
- Not human-readable.
- No required fields: an unset value reads as `0`, `""` or `false`, so
  receivers must validate values.
- No corruption check, so we add a CRC.

## Package

`package comm.protocol;` sets the C++ namespace (`comm::protocol::CameraFrame`)
and prevents name clashes with other `.proto` files. It is not sent on the
wire, so renaming it doesn't break compatibility.

## Changing a schema

- Never change or reuse a field number. Add new fields with new numbers.
- Don't change a field's type.
- Bump `comm::protocol::kProtocolVersion` only for incompatible changes.

## Usage

```cmake
target_link_libraries(my_target PRIVATE comm_protocol)
```

```cpp
#include <comm/protocol/codec.hpp>

comm::protocol::CameraFrame frame;
frame.set_protocol_version(comm::protocol::kProtocolVersion);
std::string datagram;
if (comm::protocol::encode(frame, datagram)) {
  // sendto(...)
}
```

Generated `*.pb.h` / `*.pb.cc` files go to the build directory and are not
committed.
