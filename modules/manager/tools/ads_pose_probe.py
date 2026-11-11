#!/usr/bin/env python3
"""Prints ST_RobotPose from the robot PLC over ADS, using pyads.

Independent cross-check for bin/ads_pose_probe: different ADS stack (pyads)
and its own decoding. If this works and the C++ probe doesn't, the bug is in
our code; if neither works, check the AMS route and PLC configuration.

    pip install pyads
    python3 tools/ads_pose_probe.py --symbol GVL.stRobotPose

Needs a static AMS route on the PLC for this machine (local Net ID + IP).
Linux/macOS only (pyads sets the local Net ID there).

Defaults come from ADS_* environment variables, then from modules/manager/.env
(see .env.example); command-line flags override both.
"""

import argparse
import os
import struct
import time
from pathlib import Path

import pyads
from pyads.pyads_ex import adsGetSymbolInfo

# ST_RobotPose: nCycle UDINT, then 12 LREALs (TCP xyz, TCP abc, 6 joints).
DOUBLES = struct.Struct("<12d")
PACKED_SIZE = 4 + DOUBLES.size  # 100: pack_mode 1
ALIGNED_SIZE = 8 + DOUBLES.size  # 104: 4 padding bytes after nCycle


ENV_FILE = Path(__file__).resolve().parent.parent / ".env"


def load_env_file(path: Path):
    """Minimal .env loader: KEY=VALUE lines, # comments, optional quotes.
    Variables already set in the environment take precedence."""
    if not path.is_file():
        return
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip().removeprefix("export ").strip()
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "'\"":
            value = value[1:-1]
        os.environ.setdefault(key, value)


def env_bool(name: str, default: bool = False) -> bool:
    value = os.environ.get(name)
    if value is None:
        return default
    return value.strip().lower() in ("1", "true", "yes", "on")


def decode(raw: bytes):
    if len(raw) not in (PACKED_SIZE, ALIGNED_SIZE):
        raise ValueError(
            f"Unexpected ST_RobotPose size {len(raw)} bytes; "
            f"expected {PACKED_SIZE} (packed) or {ALIGNED_SIZE} (aligned)"
        )
    (cycle,) = struct.unpack_from("<I", raw, 0)
    values = DOUBLES.unpack_from(raw, len(raw) - DOUBLES.size)
    return cycle, values[0:3], values[3:6], values[6:12]


def fmt(values, width=9):
    return " ".join(f"{v:{width}.3f}" for v in values)


def connect(args):
    plc = pyads.Connection(args.plc_net_id, args.port, args.plc_ip)
    plc.open()  # Adds the local route to the PLC on Linux/macOS.
    # pyads has no public symbol-size call; this is what get_symbol uses.
    size = adsGetSymbolInfo(plc._port, plc._adr, args.symbol).size
    print(f"{args.symbol}: {size} bytes "
          f"({'packed' if size == PACKED_SIZE else 'aligned' if size == ALIGNED_SIZE else 'UNEXPECTED'})")
    return plc, size


def main():
    load_env_file(ENV_FILE)
    env = os.environ.get

    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--symbol", default=env("ADS_SYMBOL"),
                        required=env("ADS_SYMBOL") is None,
                        help="PLC path of ST_RobotPose, e.g. GVL.stRobotPose [ADS_SYMBOL]")
    parser.add_argument("--plc-ip", default=env("ADS_PLC_IP", "192.168.137.49"),
                        help="[ADS_PLC_IP]")
    parser.add_argument("--plc-net-id", default=env("ADS_PLC_NET_ID", "192.168.137.49.1.1"),
                        help="[ADS_PLC_NET_ID]")
    parser.add_argument("--port", type=int, default=int(env("ADS_PORT", "852")),
                        help="AMS port (852 = PLC runtime 2) [ADS_PORT]")
    parser.add_argument("--local-net-id", default=env("ADS_LOCAL_NET_ID", "192.168.137.168.1.1"),
                        help="AMS Net ID of this machine (must match the PLC route) [ADS_LOCAL_NET_ID]")
    parser.add_argument("--rate", type=float, default=float(env("ADS_RATE", "10.0")),
                        help="Polls per second [ADS_RATE]")
    parser.add_argument("--raw", action=argparse.BooleanOptionalAction,
                        default=env_bool("ADS_RAW"),
                        help="Also print the raw bytes [ADS_RAW]")
    args = parser.parse_args()

    pyads.open_port()
    pyads.set_local_address(args.local_net_id)
    print(f"Connecting to {args.plc_ip} ({args.plc_net_id}) port {args.port} "
          f"as {args.local_net_id}, symbol {args.symbol}")

    plc, size, previous = None, 0, None
    period = 1.0 / args.rate
    try:
        while True:
            started = time.monotonic()
            try:
                if plc is None:
                    plc, size = connect(args)
                raw = bytes(plc.read_by_name(args.symbol, pyads.PLCTYPE_BYTE * size))
                cycle, tcp, abc, joints = decode(raw)
                step = 0 if previous is None else (cycle - previous) & 0xFFFFFFFF
                previous = cycle
                print(f"cycle {cycle:>10} (+{step:<4}) tcp [{fmt(tcp)}] "
                      f"abc [{fmt(abc)}] joints [{fmt(joints, 8)}]", flush=True)
                if args.raw:
                    print("  raw", raw.hex(" "), flush=True)
            except pyads.ADSError as error:
                print(f"ADS failed, reconnecting: {error}", flush=True)
                if plc is not None:
                    plc.close()
                plc, previous = None, None
            time.sleep(max(0.0, period - (time.monotonic() - started)))
    except KeyboardInterrupt:
        pass
    finally:
        if plc is not None:
            plc.close()


if __name__ == "__main__":
    main()
