"""Downloads Mixamo exports whose signed addresses this session's transcript shows as lines 'NAME::https://...'
(see __mxNamed in Tools/Blender/mixamo_export.js), or, with names given on the command line, the last plain
addresses in the order they were exported. Addresses are valid for 300 seconds: run it right after the export.
    python Tools/Blender/mixamo_fetch_named.py <folder> [name1 name2 ...]"""
import datetime
import re
import sys
import urllib.request
from pathlib import Path

folder = Path(sys.argv[1])
folder.mkdir(parents=True, exist_ok=True)
names = sys.argv[2:]
transcripts = Path(r"C:\Users\Julian\.claude\projects\C--Dev-Sunta-3D-Game-Engine")
newest = max(transcripts.glob("*.jsonl"), key=lambda p: p.stat().st_mtime)
text = newest.read_text(encoding="utf-8", errors="ignore").replace("\\u0026", "&")
# Line breaks inside a tool result are escaped once or twice in the transcript
text = text.replace("\\\\n", "\n").replace("\\n", "\n")
URL = r"https://mixamo-storage-prod[A-Za-z0-9%&=._~/?:+-]+?X-Amz-Signature=[0-9a-f]{64}"
now = datetime.datetime.now(datetime.timezone.utc)

if names:
    urls = []
    for url in re.findall(URL, text):
        if url not in urls:
            urls.append(url)
    pairs = list(zip(names, urls[-len(names):]))
else:
    seen = {}
    for name, url in re.findall(r"([A-Za-z0-9_]+)::(" + URL + ")", text):
        seen[name] = url
    pairs = list(seen.items())

for name, url in pairs:
    target = folder / (name + ".fbx")
    stamp = re.search(r"X-Amz-Date=(\d{8}T\d{6})Z", url)
    made = datetime.datetime.strptime(stamp.group(1), "%Y%m%dT%H%M%S").replace(tzinfo=datetime.timezone.utc)
    if target.exists() or (now - made).total_seconds() > 295:
        continue
    try:
        data = urllib.request.urlopen(url, timeout=120).read()
        target.write_bytes(data)
        print("saved", target.name, len(data))
    except Exception as error:
        print("failed", name, error)
