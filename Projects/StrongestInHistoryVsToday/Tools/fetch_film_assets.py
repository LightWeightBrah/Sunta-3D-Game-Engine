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
    res/Sounds/Film/intro.ogg, fight.ogg
    res/Sounds/Voice/solid_baby_oil.ogg, divine_baby_oil_evolve.ogg, baby_oil_detonation.ogg
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

# The moments of the film shown in the game (seconds of the film)
VIDEOS = {
    "intro.mpg":        (0.0, 36.2),     # the opening
    "domain_smile.mpg": (94.72, 97.10),  # his smile, the white eyes, the black shape before the blast
    "summon_wheel.mpg": (156.6, 160.3),  # the wheel that turns before the summoning
}

# The voices of the player's techniques (seconds of the film), without the music
VOICES = {
    "solid_baby_oil.ogg":         (51.70, 53.30),  # "Solid baby oil" (without the "unleash" after it)
    "divine_baby_oil_evolve.ogg": (138.85, 142.1),
    "baby_oil_detonation.ogg":    (107.0, 110.7),
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
    make_soundtrack(film, song)
    if not arguments.no_voices:
        make_voices(film)
    print("FILM_ASSETS_DONE")


if __name__ == "__main__":
    main()
