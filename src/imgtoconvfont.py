from sys import exit
from pathlib import Path
import argparse
from PIL import Image, ImageColor
from itertools import product

def normalize_to_rgba(
    color: tuple[int, int, int] | tuple[int, int, int, int],
    default_alpha=255
) -> tuple[int, int, int, int]:
    if len(color) == 3:
        return (*color, default_alpha)
    return color


def parse_glyph_spec(value: str) -> tuple[int, tuple[int, int]]:
    try:
        key, coords = value.split(':', 1)
        x_str, y_str = coords.split(',', 1)
        x = int(x_str)
        y = int(y_str)
        if key.startswith('U+'):
            codepoint = int(key[2:], 16)
        elif key.startswith('0x'):
            codepoint = int(key, 16)
        elif len(key) == 1:
            codepoint = ord(key)
        else:
            codepoint = int(key)
        return codepoint, (x, y)
    except Exception as exc:
        raise argparse.ArgumentTypeError(
            "Glyph spec must be CODEPOINT:X,Y where CODEPOINT is a char, decimal, 0xHEX or U+HEX"
        ) from exc

parser = argparse.ArgumentParser(
    prog='convfontimg',
    description='Make font file from image'
)

parser.add_argument('source', type=Path)
# parser.add_argument('output', type=Path)
parser.add_argument('width', type=int)
parser.add_argument('height', type=int)
parser.add_argument('linecount', type=int)
parser.add_argument('--x', type=int, default=0)
parser.add_argument('--y', type=int, default=0)
parser.add_argument('--xspacing', type=int, default=0)
parser.add_argument('--yspacing', type=int, default=0)
parser.add_argument('--baseline', type=int)
parser.add_argument('--xheight', type=int, default=0)
parser.add_argument('--codepoint', type=ord, default=ord(' '))
parser.add_argument('--lastcode', type=ord, default=0x7F)
parser.add_argument('--bgcolor', action='extend', nargs='+')
parser.add_argument('--glyph', action='append', type=parse_glyph_spec, default=[],
                    metavar='CODEPOINT:X,Y',
                    help='Specify a custom source location for a glyph. Repeatable. Glyphs may extend beyond the specified codepoint/lastcode range.')

args: argparse.Namespace = parser.parse_args()
glyph_overrides = dict(args.glyph)

im: Image.Image
with Image.open(args.source) as f:
    if 'P' == f.mode:
        im = f.convert('RGB')
    elif 'PA' == f.mode:
        im = f.convert('RGBA')
    else:
        im = f
    if args.bgcolor is not None:
        bg_colors = [ImageColor.getcolor(c, im.mode) for c in args.bgcolor]
    else:
        bg_colors = []
    print('convfont')
    print('Double width: false')
    print('Height:', args.height)
    print('Fixed width:', args.width)
    print('x-height:', args.xheight)
    print('Baseline:', args.height if args.baseline is None else args.baseline)
    print('Font data:')
    print(f'Codepoint: \'{chr(args.codepoint)}\'')
    for char in frozenset().union(range(args.codepoint, args.lastcode + 1), glyph_overrides):
        print("Data:")
        if char in glyph_overrides:
            xs, ys = glyph_overrides[char]
        else:
            index = char - args.codepoint
            xs = args.x + (index % args.linecount) * (args.width + args.xspacing)
            ys = args.y + (index // args.linecount) * (args.height + args.yspacing)
        for yi in range(args.height):
            for xi in range(args.width):
                pixel = im.getpixel((xs + xi, ys + yi))
                print(' ' if pixel in bg_colors else '*', end='')
            print()