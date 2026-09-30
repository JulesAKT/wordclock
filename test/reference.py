"""Prints the same per-minute dump as dump_faces.cpp, but using the original
MicroPython firmware's own build_time_word_data(), lifted out of main.py.

Also prints the static rainbow colours from ws2812b_matrix.get_rainbow_array().

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


main()
