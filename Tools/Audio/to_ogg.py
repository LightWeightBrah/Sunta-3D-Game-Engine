"""
Turns any sound (.wav, .mp3, .flac, .ogg, ...) into an Ogg Vorbis file the engine plays (Audio, an Audio Source, Character
Sounds, music). Uses Blender's sound library (aud), so nothing else has to be installed (no ffmpeg)
A long .wav (music) becomes about ten times smaller, so the built game doesn't grow by tens of megabytes

Run (from the repository root):
    blender --background --factory-startup --python Tools/Audio/to_ogg.py -- <input sound> <output .ogg> [--mono] [--trim] [--loudness -20] [--volume 1.0] [--seconds 0]

Blender's path on this machine: C:\\Program Files (x86)\\Steam\\steamapps\\common\\Blender\\blender.exe
'--mono' mixes the channels into one: a spatial sound (heard from a point in the world) needs only one, music keeps two
'--trim' cuts the silence at the start: a sword's hit is heard the moment it lands, not a moment later
'--loudness' brings the sound to this loudness (dB below full scale, measured over its audible part), so sounds from
different recordings sit together in one game: -20 for short sounds of a fight, quieter for ambience and music
'--volume' makes it louder or quieter, '--seconds' cuts it to this length (0 = the whole sound)
A sound louder than full scale (mixing two channels into one adds them up) is turned down so it never crackles
Prints the length of the result, so a sound can be checked without listening to it
"""

import argparse
import os
import sys

import aud
import numpy

SAMPLE_RATE  = 44100
BITRATE      = 160000
MONO         = 1
STEREO       = 2

# Quieter than this part of the loudest sample counts as silence ('--trim')
SILENCE_FRACTION = 0.03

# Where '--loudness' measures a sound: only its audible part (a short hit has much silence around it)
AUDIBLE_FRACTION = 0.1

# The loudest sample of the result, a little below full scale (above it the sound crackles)
FULL_SCALE_HEADROOM = 0.95


def parse_arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--mono", action="store_true")
    parser.add_argument("--trim", action="store_true")
    parser.add_argument("--loudness", type=float, default=None)
    parser.add_argument("--volume", type=float, default=1.0)
    parser.add_argument("--seconds", type=float, default=0.0)
    return parser.parse_args(argv)


# The moment the sound really starts (seconds): the first sample that isn't silence
def find_start_seconds(sound):
    samples = numpy.abs(sound.data())
    loudest = samples.max()
    if loudest <= 0.0:
        return 0.0

    loud_frames = numpy.nonzero(samples.max(axis=1) > loudest * SILENCE_FRACTION)[0]
    return loud_frames[0] / SAMPLE_RATE


# The volume that brings the audible part of the sound to 'loudness' dB below full scale
def volume_for_loudness(sound, loudness):
    samples = numpy.abs(sound.data()).max(axis=1)
    audible = samples[samples > samples.max() * AUDIBLE_FRACTION]
    if len(audible) == 0:
        return 1.0

    current = 20.0 * numpy.log10(float(numpy.sqrt(numpy.mean(audible ** 2))))
    return float(10.0 ** ((loudness - current) / 20.0))


# Turns the sound down when its loudest sample is beyond full scale
def keep_below_full_scale(sound):
    loudest = float(numpy.abs(sound.data()).max())
    return sound.volume(FULL_SCALE_HEADROOM / loudest) if loudest > FULL_SCALE_HEADROOM else sound


def main():
    arguments = parse_arguments()
    source = os.path.abspath(arguments.input)
    target = os.path.abspath(arguments.output)

    convert(source, target, arguments.mono, arguments.trim, arguments.volume, arguments.seconds, arguments.loudness)


# Also usable from another Blender script (import to_ogg): to_ogg.convert(source, target, is_mono=True, trim=True)
def convert(source, target, is_mono=False, trim=False, volume=1.0, seconds=0.0, loudness=None):
    channels = MONO if is_mono else STEREO

    sound = aud.Sound(source).resample(SAMPLE_RATE, False).rechannel(channels).volume(volume)
    start = find_start_seconds(sound) if trim else 0.0
    end   = start + seconds if seconds > 0.0 else -1.0
    if start > 0.0 or end > 0.0:
        sound = sound.limit(start, end)

    if loudness is not None:
        sound = sound.volume(volume_for_loudness(sound, loudness))

    sound = keep_below_full_scale(sound)

    os.makedirs(os.path.dirname(target), exist_ok=True)
    sound.write(target, rate=SAMPLE_RATE, channels=channels, format=aud.FORMAT_S16,
                container=aud.CONTAINER_OGG, codec=aud.CODEC_VORBIS, bitrate=BITRATE)

    length = aud.Sound(target).length / SAMPLE_RATE
    print(f"to_ogg: {target} ({length:.2f} s, {os.path.getsize(target) // 1024} KB)")


if __name__ == "__main__":
    main()
