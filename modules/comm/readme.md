# Comm

Communication between Safety Mesh modules, as two CMake targets:

| Target | Namespace | Contents | Depends on |
|---|---|---|---|
| `comm_protocol` | `comm::protocol` | Message schemas and codec | protobuf |
| `comm_transport` | `comm::transport` | UDP datagrams as raw bytes | POSIX sockets |

`comm_protocol` defines what is sent and has no sockets. `comm_transport`
moves bytes and knows nothing about the format. Link only what you use.
No application logic lives here.

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

Generated `*.pb.h` / `*.pb.cc` files go to the build directory and are not
committed.

## Transport

`UdpSender` sends to one fixed host and port. `UdpReceiver` binds a local
port and waits for one datagram with a timeout.

- `send` returning `true` only means the OS accepted the datagram. UDP can
  drop, duplicate or reorder datagrams without telling the sender.
- `send` returning `false` can report an error from an earlier datagram,
  often that nothing is listening.
- `receive` returns `Timeout` when nothing arrives in time or a signal
  interrupts the wait, so a loop can check for shutdown.
- `receive` returns `Truncated` when a datagram is larger than the buffer.
  Use a buffer of `comm::protocol::kMaxDatagramSize` bytes.

## Usage

Sender (cam_node):

```cmake
target_link_libraries(my_target PRIVATE comm_protocol comm_transport)
```

```cpp
#include <comm/protocol/codec.hpp>
#include <comm/transport/udp.hpp>

comm::transport::UdpSender sender("192.168.1.10", 5000);

comm::protocol::CameraFrame frame;
frame.set_protocol_version(comm::protocol::kProtocolVersion);
std::string datagram;
if (comm::protocol::encode(frame, datagram) && !sender.send(datagram)) {
  // Count or log the failure.
}
```

Receiver (manager):

```cpp
comm::transport::UdpReceiver receiver(5000);
std::array<char, comm::protocol::kMaxDatagramSize> buffer;

const auto result = receiver.receive(buffer.data(), buffer.size(),
                                     std::chrono::milliseconds(100));
comm::protocol::CameraFrame frame;
if (result.status == comm::transport::ReceiveStatus::Ok &&
    comm::protocol::decode(buffer.data(), result.size, frame) ==
        comm::protocol::DecodeError::None) {
  // Check sequence and timestamp, then use the frame.
}
```

## Tests

```sh
cmake -S . -B build && cmake --build build
ctest --test-dir build --output-on-failure
```

`comm_udp_loopback_test` sends real datagrams over `127.0.0.1`: a full
encode/send/receive/decode round trip, timeouts, truncation and a corrupted
datagram that `decode` rejects. `comm_protocol_datagram_demo` prints an
encoded frame byte by byte.
