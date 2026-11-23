"""
Makes the files of 'Strongest in History Vs Today' that are somebody else's work and so are not in the repository:
the film's opening and two moments of it (videos), its soundtrack running on into the song it uses, and the voices
of the techniques (cut out of the film without its music). They are downloaded from YouTube and cut exactly as the
game expects them, so a fresh clone looks and sounds the same as the author's copy.

Run once after cloning (from the repository root):
    python -m pip install -r Projects/StrongestInHistoryVsToday/Tools/requirements_film.txt
    python Projects/StrongestInHistoryVsToday/Tools/fetch_film_assets.py

Writes (all of them in .gitignore):
    res/Videos/intro.mpg, domain_smile.mpg, summon_wheel.mpg
    res/Sounds/Film/intro.ogg, fight.ogg, fight_music.ogg (the song alone, from the second the fight starts)
    res/Sounds/Voice/solid_baby_oil.ogg, divine_baby_oil_evolve.ogg, baby_oil_detonation.ogg, epstein_*.ogg
The downloads are kept in Tools/.film_cache (also ignored), so running it again only cuts.

Sources: the film "Epstein vs Diddy" (Artificial Productions, youtube FvMWj1UzyhA) and "Lady Gaga - Judas (Official
Music Video)" (youtube wagn8Wrmzuc), whose music the film uses: the film's sound is that song shifted by 34.30 s.
"""

import argparse
import os
import subprocess
import sys
import tempfile

TOOLS = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.dirname(TOOLS)
RES = os.path.join(PROJECT, "res")
CACHE = os.path.join(TOOLS, ".film_cache")

FILM_URL = "https://www.youtube.com/watch?v=FvMWj1UzyhA"
SONG_URL = "https://www.youtube.com/watch?v=wagn8Wrmzuc"

# The fight's soundtrack: the film from the end of the opening to where it cuts away from the song, then the song
# itself from the same moment of it (the film's sound is the song 34.30 s later), crossfaded over a second
FIGHT_FROM, FIGHT_TO = 36.2, 155.0
# The author's own cuts started this much earlier in the film's sound (cut by seeking, which lands on a whole AAC
# frame): kept, so the game's timings measured on them stay right
SEEK_LEAD = 0.036
SONG_FROM = 188.30
# The fight's music alone: the song from the second of it the fight starts on (film 36.2 s + 34.28 s), so the
# soundtrack's clock is the film's and the characters' lines are played by themselves (VOICES)
FIGHT_MUSIC_FROM = 70.482

# The moments of the film shown in the game (seconds of the film)
VIDEOS = {
    "intro.mpg":        (0.0, 36.2),     # the opening
    "domain_smile.mpg": (94.72, 97.10),  # his smile, the white eyes, the black shape before the blast
    "summon_float.mpg": (151.4, 156.6),  # he hangs in the air on his back over the burning city, then his face close
    "summon_wheel.mpg": (156.6, 161.0),  # the wheel that turns before the summoning, him falling into the white
}

# The voices of the player's techniques (seconds of the film), without the music
VOICES = {
    "solid_baby_oil.ogg":         (51.65, 54.60),  # "Solid baby oil, unleash!"
    "divine_baby_oil_evolve.ogg": (138.85, 142.1),
    "baby_oil_detonation.ogg":    (107.0, 110.7),
    # The boss's lines (played by boss_fight.lua at the film's moments)
    "epstein_domain_expansion.ogg":     (81.25, 84.7),
    "epstein_better_than_expected.ogg": (150.85, 153.4),
    "epstein_treasure_summon.ogg":      (153.9, 156.6),
    "epstein_president_trump.ogg":      (157.1, 160.85),
}
# Seconds of sound around each voice given to the separation (it hears the music better with some context)
VOICE_MARGIN = 3.0
# As loud as the voices were made for the game (quieter than the separated track)
VOICE_VOLUME = 0.42


def ffmpeg():
    import imageio_ffmpeg
    return imageio_ffmpeg.get_ffmpeg_exe()


def run(arguments):
    print(" ".join(arguments[:1] + [str(a) for a in arguments[1:]])[:200])
    subprocess.run([str(a) for a in arguments], check=True)


def download(url, stem, format_choice):
    """Downloads once into the cache; returns the file"""
    os.makedirs(CACHE, exist_ok=True)
    for name in os.listdir(CACHE):
        if name.startswith(stem + "."):
            return os.path.join(CACHE, name)

    run([sys.executable, "-m", "yt_dlp", "--no-playlist", "-f", format_choice, "--ffmpeg-location", ffmpeg(),
         "-o", os.path.join(CACHE, stem + ".%(ext)s"), url])
    return next(os.path.join(CACHE, name) for name in os.listdir(CACHE) if name.startswith(stem + "."))


def make_videos(film):
    folder = os.path.join(RES, "Videos")
    os.makedirs(folder, exist_ok=True)
    for name, (start, end) in VIDEOS.items():
        # MPEG-1, no sound (the engine's video player, see Sunta/src/Sunta/Renderer/VideoPlayer.h)
        run([ffmpeg(), "-y", "-loglevel", "error", "-ss", start, "-to", end, "-i", film,
             "-vf", "scale=1280:720,fps=30", "-c:v", "mpeg1video", "-q:v", 4, "-an", os.path.join(folder, name)])


# The film's own sound (voices, music, blasts) for the staged moments the game plays in step with the film
# (boss_fight.lua O.playFilmAudio): the opening of his domain, the lines before his summoning
FILM_AUDIO = {
    "domain_opening_film.ogg": (74.8, 102.6),
    "summoning_film.ogg":      (150.6, 161.0),
}


def make_film_audio(film):
    folder = os.path.join(RES, "Sounds", "Film")
    os.makedirs(folder, exist_ok=True)
    for name, (start, end) in FILM_AUDIO.items():
        run([ffmpeg(), "-y", "-loglevel", "error", "-ss", start, "-to", end, "-i", film, "-vn", "-ac", 2, "-ar", 44100,
             "-c:a", "libvorbis", "-q:a", 6, os.path.join(folder, name)])


def make_soundtrack(film, song):
    folder = os.path.join(RES, "Sounds", "Film")
    os.makedirs(folder, exist_ok=True)
    run([ffmpeg(), "-y", "-loglevel", "error", "-t", VIDEOS["intro.mpg"][1], "-i", film, "-vn",
         "-c:a", "libvorbis", "-q:a", 5, os.path.join(folder, "intro.ogg")])
    # (the cuts of sound are made sample-exact in the filter, not by seeking: seeking jumps by whole AAC frames)
    run([ffmpeg(), "-y", "-loglevel", "error", "-i", film, "-i", song, "-filter_complex",
         "[0:a]atrim={}:{},asetpts=PTS-STARTPTS,aresample=44100[a];".format(FIGHT_FROM - SEEK_LEAD, FIGHT_TO - SEEK_LEAD)
         + "[1:a]atrim=start={},asetpts=PTS-STARTPTS,aresample=44100[b];".format(SONG_FROM)
         + "[a][b]acrossfade=d=1.0:c1=tri:c2=tri[out]",
         "-map", "[out]", "-vn", "-c:a", "libvorbis", "-q:a", 5, os.path.join(folder, "fight.ogg")])
    run([ffmpeg(), "-y", "-loglevel", "error", "-i", song, "-af",
         # (without the song's silent end: the game loops it, a long silence came round on every loop)
         "atrim=start={},asetpts=PTS-STARTPTS,afade=t=in:d=0.3,atrim=0:244.2,afade=t=out:st=242.4:d=1.8,aresample=44100".format(FIGHT_MUSIC_FROM),
         "-vn", "-c:a", "libvorbis", "-q:a", 6, os.path.join(folder, "fight_music.ogg")])


def remove_music_bed(vocals, rate):
    try:
        import noisereduce
        import numpy
    except ImportError:
        return vocals
    mono = vocals.mean(axis=1)
    frame = int(rate * 0.05)
    frames = [mono[i:i + frame] for i in range(0, len(mono) - frame, frame)]
    energies = numpy.array([numpy.sqrt((f ** 2).mean()) for f in frames])
    quiet = numpy.argsort(energies)[:max(4, len(frames) // 5)]
    noise = numpy.concatenate([frames[i] for i in sorted(quiet)])
    clean = noisereduce.reduce_noise(y=mono, sr=rate, y_noise=noise, stationary=False, prop_decrease=0.95, n_fft=2048)
    clean = noisereduce.reduce_noise(y=clean, sr=rate, y_noise=noise * 0.5, stationary=True, prop_decrease=0.75, n_fft=2048)
    clean *= numpy.abs(mono).max() / (numpy.abs(clean).max() + 1e-9)
    return numpy.stack([clean, clean], axis=1)


def make_voices(film):
    import soundfile
    import torch
    from demucs.apply import apply_model
    from demucs.pretrained import get_model

    folder = os.path.join(RES, "Sounds", "Voice")
    os.makedirs(folder, exist_ok=True)
    # The fine-tuned model, run twice (on the voices of the first run): one pass left the music's beat in the voices
    model = get_model("htdemucs_ft")
    model.eval()
    vocals_index = model.sources.index("vocals")

    with tempfile.TemporaryDirectory() as work:
        for name, (start, end) in VOICES.items():
            start -= SEEK_LEAD
            end -= SEEK_LEAD
            begin = max(start - VOICE_MARGIN, 0.0)
            raw = os.path.join(work, "raw.wav")
            run([ffmpeg(), "-y", "-loglevel", "error", "-i", film, "-vn",
                 "-af", "atrim={}:{},asetpts=PTS-STARTPTS".format(begin, end + VOICE_MARGIN),
                 "-ac", 2, "-ar", model.samplerate, raw])

            audio, rate = soundfile.read(raw, dtype="float32", always_2d=True)
            wave = torch.from_numpy(audio.T)
            mean, std = wave.mean(), wave.std()
            for _ in range(2):
                with torch.no_grad():
                    stems = apply_model(model, ((wave - mean) / std)[None], device="cpu", split=True, overlap=0.25)[0]
                wave = stems[vocals_index] * std + mean
                mean, std = wave.mean(), wave.std()
            vocals = wave.numpy().T

            # What music is left under the voice: the quietest stretches between the words are its profile, taken away
            # (noisereduce, MIT; skipped when it is not installed)
            vocals = remove_music_bed(vocals, rate)

            separated = os.path.join(work, "vocals.wav")
            soundfile.write(separated, vocals, rate)
            # The voice alone, mono, as the game plays it
            run([ffmpeg(), "-y", "-loglevel", "error", "-i", separated,
                 "-af", "atrim={}:{},asetpts=PTS-STARTPTS,volume={}".format(start - begin, end - begin, VOICE_VOLUME),
                 "-ac", 1, "-ar", 44100, "-c:a", "libvorbis", "-q:a", 4, os.path.join(folder, name)])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--no-voices", action="store_true", help="skip the voices (no Demucs needed)")
    parser.add_argument("--only-voices", action="store_true", help="only cut the voices again (the film is in the cache)")
    arguments = parser.parse_args()

    film = download(FILM_URL, "film", "bv*[height<=720]+ba/b[height<=720]/b")
    song = download(SONG_URL, "song", "ba[ext=m4a]/ba")

    if arguments.only_voices:
        make_voices(film)
        print("FILM_ASSETS_DONE")
        return

    make_videos(film)
    make_film_audio(film)
    make_soundtrack(film, song)
    if not arguments.no_voices:
        make_voices(film)
    print("FILM_ASSETS_DONE")


if __name__ == "__main__":
    main()
