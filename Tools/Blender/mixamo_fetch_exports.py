"""Downloads the Mixamo exports whose signed addresses appear in this session's transcript and are still valid."""
import datetime
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

folder = Path(sys.argv[1])
folder.mkdir(parents=True, exist_ok=True)
transcripts = Path(r"C:\Users\Julian\.claude\projects\C--Dev-Sunta-3D-Game-Engine")
newest = max(transcripts.glob("*.jsonl"), key=lambda p: p.stat().st_mtime)
text = newest.read_text(encoding="utf-8", errors="ignore").replace("\\u0026", "&")

now = datetime.datetime.now(datetime.timezone.utc)
seen = set()
for url in re.findall(r"https://mixamo-storage-prod[A-Za-z0-9%&=._~/?:+-]+", text):
    stamp = re.search(r"X-Amz-Date=(\d{8}T\d{6})Z", url)
    name = re.search(r"filename%3D%22(.+?)\.fbx%22", url)
    if not stamp or not name or not url.endswith(tuple("0123456789abcdef")) or url in seen:
        continue
    seen.add(url)
    made = datetime.datetime.strptime(stamp.group(1), "%Y%m%dT%H%M%S").replace(tzinfo=datetime.timezone.utc)
    file_name = urllib.parse.unquote(name.group(1)) + ".fbx"
    target = folder / file_name
    if (now - made).total_seconds() > 290 or target.exists():
        continue
    try:
        data = urllib.request.urlopen(url, timeout=120).read()
        target.write_bytes(data)
        print("saved", file_name, len(data))
    except Exception as error:
        print("failed", file_name, error)
