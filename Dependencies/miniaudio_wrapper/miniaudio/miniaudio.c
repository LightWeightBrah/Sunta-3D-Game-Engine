// miniaudio (https://github.com/mackron/miniaudio, public domain or MIT-0, by David Reid): audio playback in one header.
// Its implementation is compiled here, once; the rest of the engine includes only the declarations
// stb_vorbis (Sean Barrett, public domain or MIT, the copy from miniaudio's "extras") adds .ogg files to the formats
// miniaudio reads itself (.wav, .flac, .mp3). Its declarations go before miniaudio, its implementation after it
#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"

#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
