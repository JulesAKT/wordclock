"""Prints the same per-minute dump as dump_faces.cpp, but using the original
MicroPython firmware's own build_time_word_data(), lifted out of main.py.

Also prints the static rainbow colours from ws2812b_matrix.get_rainbow_array(),
and matrix rain frames from matrix_rain.py (scenarios as in dump_faces.cpp).

Usage: python3 reference.py path/to/upstream/src
"""
import ast
import sys
from types import SimpleNamespace

NEEDED = {'clockFont', 'build_time_word_data', 'merge_chars', 'merge_color_array'}

# Sentinel colours: each word category gets a colour equal to its Part value.
PAST_TO, HOUR, MINUTE = (1, 0, 0), (2, 0, 0), (3, 0, 0)


def load(main_py):
    tree = ast.parse(open(main_py).read())
    nodes = []
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name in NEEDED:
            nodes.append(node)
        elif isinstance(node, ast.Assign) and any(
                isinstance(t, ast.Name) and t.id in NEEDED for t in node.targets):
            nodes.append(node)
    namespace = {
        'DISPLAY_MODE_MATRIX_RAIN': 'matrix_rain',
        'minute_color': MINUTE,
        'hour_color': HOUR,
        'past_to_color': PAST_TO,
        'app_state': SimpleNamespace(),
    }
    exec(compile(ast.Module(body=nodes, type_ignores=[]), main_py, 'exec'), namespace)
    return namespace


def load_rainbow(matrix_py):
    tree = ast.parse(open(matrix_py).read())
    cls = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name == 'ws2812b_matrix')
    methods = [n for n in cls.body if isinstance(n, ast.FunctionDef) and n.name in ('wheel', 'get_rainbow_array')]
    namespace = {}
    exec(compile(ast.Module(body=methods, type_ignores=[]), matrix_py, 'exec'), namespace)
    matrix = SimpleNamespace(width=8, height=8)
    matrix.wheel = lambda pos: namespace['wheel'](matrix, pos)
    return namespace['get_rainbow_array'](matrix)


RAIN_SCENARIOS = [
    # white_head, rain_over_words, trail_length, spawn_rate, brightness_level, background
    (False, False, 4, 18, 2, (0, 255, 0)),
    (True, True, 4, 18, 2, (0, 255, 0)),
    (True, False, 8, 60, 15, (0, 200, 255)),
    (False, True, 1, 100, 7, (255, 64, 0)),
    (True, True, 6, 5, 3, (0, 255, 0)),
]
RAIN_FRAMES = 200
TIME_BRIGHTNESS_CAP = 3


class Lcg:
    """Same pseudo-random sequence as lcg() in dump_faces.cpp."""

    def __init__(self):
        self.state = 1

    def randint(self, low, high):
        self.state = (self.state * 1103515245 + 12345) & 0x7fffffff
        return low + self.state % (high - low + 1)


def dump_rain(src, ns):
    sys.path.insert(0, src)
    import matrix_rain
    ns['app_state'] = SimpleNamespace(
        matrix_rain_hour_color=(255, 255, 0),
        matrix_rain_minute_color=(255, 0, 0),
        matrix_rain_past_to_color=(255, 128, 0))
    shown = {}
    matrix = SimpleNamespace(
        set_brightness=lambda level: None,
        show_char_with_color_array=lambda char, colours: shown.update(char=char, colours=colours))
    config = {'ENABLE_MAX7219': False, 'ENABLE_HT16K33': False, 'ENABLE_WS2812B': True}
    for index, (white_head, over_words, trail, spawn, level, background) in enumerate(RAIN_SCENARIOS):
        state = matrix_rain.MatrixRainState()
        rng = Lcg()
        for frame in range(RAIN_FRAMES):
            t = index * 97 + frame * 3
            hour, minute = t // 60 % 24, t % 60
            ns['get_corrected_time'] = lambda h=hour, m=minute: (2026, 1, 1, h, m, 0, 3, 1)
            state.advance(rng, spawn, trail)
            matrix_rain.render(
                state, background_color=background, white_head=white_head, affect_time=over_words,
                trail_length=trail, time_brightness_cap=TIME_BRIGHTNESS_CAP, brightness=level,
                config=config, spi_matrix=None, i2c_matrix=None, ws2812b_matrix=matrix,
                current_display_minute_key=lambda h=hour, m=minute: (h, m),
                build_time_word_data=ns['build_time_word_data'], matrix_mode='matrix_rain')
            pixels = []
            for led in range(64):
                lit = shown['char'][led // 8] & (1 << (7 - led % 8))
                pixels.append('%02x%02x%02x' % (shown['colours'][led] if lit else (0, 0, 0)))
            print('rain %d %d %s' % (index, frame, ''.join(pixels)))


def main():
    src = sys.argv[1]
    ns = load(src + '/main.py')
    for h in range(24):
        for m in range(60):
            ns['get_corrected_time'] = lambda h=h, m=m: (2026, 1, 1, h, m, 0, 3, 1)
            _, colours, _ = ns['build_time_word_data']()
            print('%02d:%02d %s' % (h, m, ''.join(str(c[0]) for c in colours)))
    for led, (r, g, b) in enumerate(load_rainbow(src + '/ws2812b_matrix.py')):
        print('rainbow %d %d %d %d' % (led, r, g, b))
    dump_rain(src, ns)


main()
