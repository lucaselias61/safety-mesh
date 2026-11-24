#!/usr/bin/env python3
"""Makes a printable ChArUco board PDF from config/charuco.json.

    pip install "opencv-python>=4.7"
    python3 tools/gen_charuco.py 500                   # board diagonal in mm
    python3 tools/gen_charuco.py 500 --update-config   # also write the sizes to the config

- Rows, columns, dictionary and marker/square ratio come from the config.
- The diagonal sets the square size. The page is the board plus --margin-mm.
- Vector PDF. Print at 100 %, then measure a square and fix the config if needed.
"""

import argparse
import json
import math
import sys
from pathlib import Path

import cv2

DEFAULT_CONFIG = Path(__file__).resolve().parent.parent / "config" / "charuco.json"
PT_PER_MM = 72 / 25.4
LABEL_MM = 3


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("diagonal_mm", type=float, help="board diagonal in mm, corner to corner of the squares")
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG, help="board config (default config/charuco.json)")
    parser.add_argument("--out", type=Path, default=Path("charuco.pdf"), help="PDF path (default charuco.pdf)")
    parser.add_argument("--margin-mm", type=float, default=10, help="white border around the board (default 10)")
    parser.add_argument("--update-config", action="store_true", help="write the new square and marker sizes to --config")
    return parser.parse_args()


def load_board(path):
    with path.open() as source:
        config = json.load(source)
    if config["marker_size_m"] >= config["square_size_m"]:
        raise ValueError("marker_size_m must be smaller than square_size_m.")
    return config


def board_sizes(config, diagonal_mm):
    # Square from the diagonal. Marker keeps the config's marker/square ratio.
    square = diagonal_mm / math.hypot(config["columns"], config["rows"])
    marker = square * config["marker_size_m"] / config["square_size_m"]
    return square, marker


def make_board(config, square, marker):
    # Same as cv::aruco::CharucoBoard in src/world_pose.cpp. Units are mm here.
    name = config["dictionary"]
    if not hasattr(cv2.aruco, name):
        raise ValueError(f"Unknown dictionary {name}.")
    dictionary = cv2.aruco.getPredefinedDictionary(getattr(cv2.aruco, name))
    board = cv2.aruco.CharucoBoard((config["columns"], config["rows"]), square, marker, dictionary)
    return board, dictionary


def black_rects(config, square, board, dictionary):
    """Returns black (x, y, w, h) rects in mm. Origin top left, y down."""
    rects = []
    # Squares holding a marker are white. The others are black.
    marker_cells = set()
    for corners in board.getObjPoints():
        x, y = corners[0][0], corners[0][1]
        marker_cells.add((int(x // square), int(y // square)))
    for row in range(config["rows"]):
        for col in range(config["columns"]):
            if (col, row) not in marker_cells:
                rects.append((col * square, row * square, square, square))

    bits_per_side = dictionary.markerSize
    for marker_id, corners in zip(board.getIds().flatten(), board.getObjPoints()):
        x0, y0 = float(corners[0][0]), float(corners[0][1])
        side = float(corners[1][0]) - x0
        cell = side / (bits_per_side + 2)
        bits = cv2.aruco.Dictionary.getBitsFromByteList(
            dictionary.bytesList[marker_id:marker_id + 1], bits_per_side)
        # One black border bit around the data bits. Bit 1 is white.
        grid = [[0] * (bits_per_side + 2) for _ in range(bits_per_side + 2)]
        for r in range(bits_per_side):
            for c in range(bits_per_side):
                grid[r + 1][c + 1] = int(bits[r][c])
        for r, line in enumerate(grid):
            # Merge black runs in a row. Fewer rects, smaller PDF.
            c = 0
            while c < len(line):
                if line[c]:
                    c += 1
                    continue
                start = c
                while c < len(line) and not line[c]:
                    c += 1
                rects.append((x0 + start * cell, y0 + r * cell, (c - start) * cell, cell))
    return rects


def pdf_bytes(rects, board_w, board_h, margin, label):
    """One page PDF, sizes in mm. Board at margin, label in the bottom margin."""
    page_w = (board_w + 2 * margin) * PT_PER_MM
    page_h = (board_h + 2 * margin) * PT_PER_MM
    # White page, so viewers do not show it transparent.
    ops = [f"1 g 0 0 {page_w:.4f} {page_h:.4f} re f", "0 g"]
    for x, y, w, h in rects:
        # PDF y is up. Flip from the top-left origin.
        left = (margin + x) * PT_PER_MM
        bottom = page_h - (margin + y + h) * PT_PER_MM
        ops.append(f"{left:.4f} {bottom:.4f} {w * PT_PER_MM:.4f} {h * PT_PER_MM:.4f} re")
    # One fill for all rects. Separate fills show seams in viewers.
    ops.append("f")
    font_pt = LABEL_MM * PT_PER_MM
    label_y = (margin - LABEL_MM) / 2 * PT_PER_MM
    if margin > LABEL_MM:
        text = label.replace("\\", "\\\\").replace("(", "\\(").replace(")", "\\)")
        ops.append(f"BT /F1 {font_pt:.2f} Tf {margin * PT_PER_MM:.4f} {label_y:.4f} Td ({text}) Tj ET")
    content = "\n".join(ops).encode()

    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        (f"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 {page_w:.4f} {page_h:.4f}] "
         f"/Resources << /Font << /F1 5 0 R >> >> /Contents 4 0 R >>").encode(),
        b"<< /Length " + str(len(content)).encode() + b" >>\nstream\n" + content + b"\nendstream",
        b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
    ]
    out = bytearray(b"%PDF-1.4\n")
    offsets = []
    for number, body in enumerate(objects, start=1):
        offsets.append(len(out))
        out += f"{number} 0 obj\n".encode() + body + b"\nendobj\n"
    xref = len(out)
    out += f"xref\n0 {len(objects) + 1}\n0000000000 65535 f \n".encode()
    for offset in offsets:
        out += f"{offset:010d} 00000 n \n".encode()
    out += f"trailer\n<< /Size {len(objects) + 1} /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n".encode()
    return bytes(out)


def update_config(path, config, square, marker):
    config["square_size_m"] = round(square / 1000, 7)
    config["marker_size_m"] = round(marker / 1000, 7)
    path.write_text(json.dumps(config, indent=2) + "\n")


def main():
    args = parse_args()
    if args.diagonal_mm <= 0 or args.margin_mm < 0:
        raise ValueError("diagonal_mm must be positive and margin_mm not negative.")
    config = load_board(args.config)
    square, marker = board_sizes(config, args.diagonal_mm)
    board, dictionary = make_board(config, square, marker)
    board_w, board_h = config["columns"] * square, config["rows"] * square

    label = (f"{config['columns']}x{config['rows']} {config['dictionary']}  "
             f"square {square:.3f} mm  marker {marker:.3f} mm  print at 100 %")
    rects = black_rects(config, square, board, dictionary)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(pdf_bytes(rects, board_w, board_h, args.margin_mm, label))

    print(f"Wrote {args.out}")
    print(f"  board  {board_w:.1f} x {board_h:.1f} mm, page {board_w + 2 * args.margin_mm:.1f} x "
          f"{board_h + 2 * args.margin_mm:.1f} mm")
    print(f"  square_size_m {square / 1000:.7f}  marker_size_m {marker / 1000:.7f}")
    if args.update_config:
        update_config(args.config, config, square, marker)
        print(f"Updated {args.config}")
    else:
        print(f"Set these in {args.config}, or rerun with --update-config.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError) as error:
        print(f"gen_charuco failed: {error}", file=sys.stderr)
        sys.exit(1)
