"""
Reads every book of books.py aloud into res/Sounds/Voice/<id>.wav: the voice of the lich of the court.

Kokoro (82M, Apache-2.0, https://huggingface.co/hexgrad/Kokoro-82M) through kokoro-onnx (MIT); the model files
kokoro-v1.0.onnx and voices-v1.0.bin (github.com/thewh1teagle/kokoro-onnx releases) in AIContext/tts_models
(not in the repository). Python 3.13 venv:
    python -m venv AIContext/venv_tts && AIContext/venv_tts/Scripts/python -m pip install kokoro-onnx soundfile
    AIContext/venv_tts/Scripts/python Projects/GraveCrown/Tools/narrate_books.py
The voice is lowered a little and given a faint echo of a crypt
"""

import os
import sys

import numpy
import soundfile
from kokoro_onnx import Kokoro

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from books import BOOKS, SPEED, VOICE  # noqa: E402

ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
MODELS = os.path.join(ROOT, "AIContext", "tts_models")
OUT = os.path.join(HERE, "..", "res", "Sounds", "Voice")

# Lower: the clip played this much slower (resampled), a deeper and older voice
LOWER = 0.9
# The crypt: a few faint, late repeats of the voice
ECHOES = ((0.09, 0.22), (0.19, 0.12), (0.31, 0.06))


def lowered(samples, factor):
    positions = numpy.arange(0, len(samples) - 1, factor)
    return numpy.interp(positions, numpy.arange(len(samples)), samples)


def with_echo(samples, rate):
    out = numpy.copy(samples)
    out = numpy.concatenate([out, numpy.zeros(int(rate * 0.4))])
    for delay, strength in ECHOES:
        shift = int(rate * delay)
        out[shift:shift + len(samples)] += samples * strength
    return out / max(1.0, float(numpy.abs(out).max()) / 0.95)


def main():
    os.makedirs(OUT, exist_ok=True)
    kokoro = Kokoro(os.path.join(MODELS, "kokoro-v1.0.onnx"), os.path.join(MODELS, "voices-v1.0.bin"))
    for book_id, (title, _, text) in BOOKS.items():
        samples, rate = kokoro.create(title + ". " + text, voice=VOICE, speed=SPEED, lang="en-gb")
        samples = with_echo(lowered(numpy.asarray(samples, dtype=numpy.float32), LOWER), rate)
        path = os.path.join(OUT, book_id + ".wav")
        soundfile.write(path, samples, rate, subtype="PCM_16")
        print("VOICE {} {:.1f} s -> {}".format(book_id, len(samples) / rate, os.path.normpath(path)))


if __name__ == "__main__":
    main()
