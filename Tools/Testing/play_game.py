"""Runs a built game, presses keys in its window without focus and saves pictures of it.
    python play_game.py <exe> <out dir> "8:key:E" "12:shot:a" "20:key:1" ...   (seconds since start : action : argument)
The game runs muted (SUNTA_MUTE=1): the person at the computer may be listening to something else. Put --sound
before the steps to hear it.
Actions: key:<name> (a tap), hold:<name>/<seconds>, click:L or click:R (a mouse button, hold with click:R/1.5),
shot:<file name>
"""
import ctypes
import os
import subprocess
import sys
import time
from ctypes import wintypes
from pathlib import Path

from PIL import Image

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32
user32.SetProcessDPIAware()

WM_KEYDOWN, WM_KEYUP = 0x0100, 0x0101
KEYS = {"Enter": 0x0D, "Shift": 0x10, "Ctrl": 0x11, "Space": 0x20}


def find_window(pid):
    found = []

    def visit(hwnd, _):
        window_pid = wintypes.DWORD()
        user32.GetWindowThreadProcessId(hwnd, ctypes.byref(window_pid))
        if window_pid.value == pid and user32.IsWindowVisible(hwnd):
            rect = wintypes.RECT()
            user32.GetClientRect(hwnd, ctypes.byref(rect))
            name = ctypes.create_unicode_buffer(64)
            user32.GetClassNameW(hwnd, name, 64)
            if rect.right > 600 and "Console" not in name.value:
                found.append(hwnd)
        return True

    user32.EnumWindows(ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)(visit), 0)
    return found[0] if found else None


def press(hwnd, name, hold=0.08):
    code = KEYS.get(name, ord(name[0].upper()))
    scan = user32.MapVirtualKeyW(code, 0)
    user32.PostMessageW(hwnd, WM_KEYDOWN, code, 1 | (scan << 16))
    time.sleep(hold)
    user32.PostMessageW(hwnd, WM_KEYUP, code, 1 | (scan << 16) | (0xC0 << 24))


MOUSE = {"L": (0x0201, 0x0202, 0x0001), "R": (0x0204, 0x0205, 0x0002)}


def click(hwnd, button, hold=0.06):
    down, up, flag = MOUSE[button]
    rect = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    where = (rect.bottom // 2) << 16 | (rect.right // 2)
    user32.PostMessageW(hwnd, down, flag, where)
    time.sleep(hold)
    user32.PostMessageW(hwnd, up, 0, where)


def shot(hwnd, path):
    rect = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    width, height = rect.right, rect.bottom
    window_dc = user32.GetDC(hwnd)
    memory_dc = gdi32.CreateCompatibleDC(window_dc)
    bitmap = gdi32.CreateCompatibleBitmap(window_dc, width, height)
    gdi32.SelectObject(memory_dc, bitmap)
    user32.PrintWindow(hwnd, memory_dc, 3)

    class BITMAPINFOHEADER(ctypes.Structure):
        _fields_ = [("biSize", wintypes.DWORD), ("biWidth", wintypes.LONG), ("biHeight", wintypes.LONG),
                    ("biPlanes", wintypes.WORD), ("biBitCount", wintypes.WORD), ("biCompression", wintypes.DWORD),
                    ("biSizeImage", wintypes.DWORD), ("biXPelsPerMeter", wintypes.LONG), ("biYPelsPerMeter", wintypes.LONG),
                    ("biClrUsed", wintypes.DWORD), ("biClrImportant", wintypes.DWORD)]

    header = BITMAPINFOHEADER(ctypes.sizeof(BITMAPINFOHEADER), width, -height, 1, 32, 0, 0, 0, 0, 0, 0)
    pixels = ctypes.create_string_buffer(width * height * 4)
    gdi32.GetDIBits(memory_dc, bitmap, 0, height, pixels, ctypes.byref(header), 0)
    Image.frombuffer("RGBA", (width, height), pixels, "raw", "BGRA", 0, 1).convert("RGB").resize((width // 2, height // 2)).save(path)
    gdi32.DeleteObject(bitmap)
    gdi32.DeleteDC(memory_dc)
    user32.ReleaseDC(hwnd, window_dc)


def main():
    exe, out = Path(sys.argv[1]), Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    arguments = [argument for argument in sys.argv[3:] if argument != "--sound"]
    steps = sorted((float(s.split(":")[0]), s.split(":")[1], s.split(":")[2]) for s in arguments)

    environment = dict(os.environ)
    if "--sound" not in sys.argv:
        environment["SUNTA_MUTE"] = "1"

    process = subprocess.Popen([str(exe)], cwd=exe.parent, env=environment)
    start = time.time()
    hwnd = None
    while hwnd is None and time.time() - start < 40:
        time.sleep(0.5)
        hwnd = find_window(process.pid)

    for at, action, argument in steps:
        time.sleep(max(at - (time.time() - start), 0))
        if action == "key":
            press(hwnd, argument)
        elif action == "hold":
            name, seconds = argument.split("/")
            press(hwnd, name, float(seconds))
        elif action == "click":
            button, _, seconds = argument.partition("/")
            click(hwnd, button, float(seconds) if seconds else 0.06)
        elif action == "shot":
            shot(hwnd, out / f"{argument}.png")

    process.terminate()


main()
