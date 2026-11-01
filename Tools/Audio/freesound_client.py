"""
A small client of the Freesound API (https://freesound.org/docs/api/): finds CC0 sounds and downloads them

Only sounds under Creative Commons 0 are searched, so whatever it finds can go into the game and the thesis freely.
The API key is read from the environment variable FREESOUND_API_KEY (never written anywhere); on Windows a key set for the
user ([Environment]::SetEnvironmentVariable('FREESOUND_API_KEY', '...', 'User')) is found without restarting anything.
A key gives the high quality previews (Ogg Vorbis about 192 kbps), which is what a game needs.
Every download is written down in freesound_log.json (id, name, author, link), so the credits can always be written

Plain Python (no Blender needed), for example:
    python Tools/Audio/freesound_client.py search "sword whoosh" --max-seconds 2 --count 15
    python Tools/Audio/freesound_client.py download 123456 --out Downloads/whoosh.ogg
"""

import argparse
import datetime
import json
import os
import time
import urllib.error
import urllib.parse
import urllib.request

try:
    # Antivirus programs often put their own certificate in front of HTTPS: Windows trusts it, plain Python doesn't
    import truststore
    truststore.inject_into_ssl()
except ImportError:
    pass

API = "https://freesound.org/apiv2"
AUDIO_FOLDER = os.path.dirname(os.path.abspath(__file__))
LOG_PATH = os.path.join(AUDIO_FOLDER, "freesound_log.json")

CC0_FILTER = 'license:"Creative Commons 0"'
SEARCH_FIELDS = "id,name,username,duration,license,tags,avg_rating,num_ratings,num_downloads,samplerate,channels,url,previews"
PREVIEW = "preview-hq-ogg"

# Freesound allows 60 requests a minute: a little more than a second between two of them keeps far from it
SECONDS_BETWEEN_REQUESTS = 1.1
last_request_time = 0.0

# When it says "too many requests" anyway (e.g. right after another program asked): wait and try again
TOO_MANY_REQUESTS      = 429
THROTTLED_WAIT_SECONDS = 60
THROTTLED_ATTEMPTS     = 4


# ========== Requests ==========


def api_key():
    key = os.environ.get("FREESOUND_API_KEY")
    if not key and os.name == "nt":
        # Set for the user after this program's console started: read it from the registry
        import winreg
        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Environment") as environment:
                key = winreg.QueryValueEx(environment, "FREESOUND_API_KEY")[0]
        except OSError:
            key = None
    if not key:
        raise RuntimeError("FREESOUND_API_KEY is not set (make one at https://freesound.org/apiv2/apply)")
    return key


def wait_for_turn():
    global last_request_time
    time.sleep(max(0.0, last_request_time + SECONDS_BETWEEN_REQUESTS - time.time()))
    last_request_time = time.time()


def open_url(url):
    for attempt in range(THROTTLED_ATTEMPTS):
        wait_for_turn()
        req = urllib.request.Request(url, headers={"Authorization": "Token " + api_key()})
        try:
            return urllib.request.urlopen(req, timeout=60)
        except urllib.error.HTTPError as error:
            details = error.read().decode("utf-8", "ignore")
            if error.code != TOO_MANY_REQUESTS or attempt == THROTTLED_ATTEMPTS - 1:
                raise RuntimeError("Freesound {}: {} {}".format(url.split("?")[0], error.code, details))

            # Too many requests in the last minute: wait for the minute to pass
            print("Freesound is throttling the requests, waiting {} s ({})".format(THROTTLED_WAIT_SECONDS, details))
            time.sleep(THROTTLED_WAIT_SECONDS)


def get_json(path, parameters):
    with open_url(API + path + "?" + urllib.parse.urlencode(parameters)) as response:
        return json.loads(response.read())


# ========== Searching ==========


def search(query, min_seconds=0.0, max_seconds=0.0, count=15, sort="score"):
    """CC0 sounds matching the words, the best ones first ('sort': score, rating_desc, downloads_desc, duration_asc)"""
    duration_filter = "duration:[{} TO {}]".format(min_seconds, max_seconds if max_seconds > 0 else "*")
    parameters = {
        "query": query,
        "filter": CC0_FILTER + " " + duration_filter,
        "fields": SEARCH_FIELDS,
        "page_size": count,
        "sort": sort,
    }
    return get_json("/search/text/", parameters)["results"]


def get_sound(sound_id):
    return get_json("/sounds/{}/".format(sound_id), {"fields": SEARCH_FIELDS})


# ========== Downloading ==========


def write_log(entry):
    log = []
    if os.path.exists(LOG_PATH):
        with open(LOG_PATH, encoding="utf-8") as file:
            log = json.load(file)

    log = [old for old in log if old["id"] != entry["id"]] + [entry]
    with open(LOG_PATH, "w", encoding="utf-8") as file:
        json.dump(log, file, indent=2, ensure_ascii=False)


def download(sound_id, out_path):
    """The high quality preview of the sound (.ogg) into 'out_path', written down in the log"""
    sound = get_sound(sound_id)
    if "Creative Commons 0" not in sound["license"] and "publicdomain/zero" not in sound["license"]:
        raise RuntimeError("Sound {} is not CC0 ({}), it is not used".format(sound_id, sound["license"]))

    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open_url(sound["previews"][PREVIEW]) as response, open(out_path, "wb") as file:
        file.write(response.read())

    write_log({
        "id": sound["id"],
        "name": sound["name"],
        "author": sound["username"],
        "license": sound["license"],
        "url": sound["url"],
        "file": os.path.basename(out_path),
        "downloaded": datetime.date.today().isoformat(),
    })
    return sound


def describe(sound):
    return "{:>7}  {:5.2f}s  {:>4} dl  rating {:.1f}  {:<20}  {}".format(
        sound["id"], sound["duration"], sound.get("num_downloads", 0), sound.get("avg_rating", 0.0),
        sound["username"][:20], sound["name"])


def main():
    parser = argparse.ArgumentParser()
    commands = parser.add_subparsers(dest="command", required=True)

    search_command = commands.add_parser("search")
    search_command.add_argument("query")
    search_command.add_argument("--min-seconds", type=float, default=0.0)
    search_command.add_argument("--max-seconds", type=float, default=0.0)
    search_command.add_argument("--count", type=int, default=15)
    search_command.add_argument("--sort", default="score")

    download_command = commands.add_parser("download")
    download_command.add_argument("id", type=int)
    download_command.add_argument("--out", required=True)

    arguments = parser.parse_args()
    if arguments.command == "search":
        for sound in search(arguments.query, arguments.min_seconds, arguments.max_seconds, arguments.count, arguments.sort):
            print(describe(sound))
    else:
        print("downloaded", describe(download(arguments.id, arguments.out)))


if __name__ == "__main__":
    main()
