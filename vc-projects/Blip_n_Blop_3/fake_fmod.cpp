#include "fmod.h"

#include <SDL2/SDL_mixer.h>

#include "ben_debug.h"

namespace {

bool mixer_open = false;
bool logged_sfx_playback = false;

const char* CodecName(int flag) {
    switch (flag) {
        case MIX_INIT_MP3:
            return "MP3";
        case MIX_INIT_OGG:
            return "OGG";
        default:
            return "unknown";
    }
}

Mix_Music* LoadMusic(const char* filename) {
    SDL_RWops* source = SDL_RWFromFile(filename, "rb");
    if (!source) return nullptr;

    Sint64 offset = 0;
    Uint8 byte = 0;
    while (SDL_RWread(source, &byte, 1, 1) == 1 && byte == 0) ++offset;
    if (SDL_RWseek(source, offset, RW_SEEK_SET) < 0) {
        SDL_RWclose(source);
        return nullptr;
    }
    if (offset > 0) {
        debug << "Skipping " << offset << " leading padding byte(s) in "
              << filename << "\n";
    }
    return Mix_LoadMUS_RW(source, 1);
}

}  // namespace

extern "C" {
struct FMUSIC_MODULE {};
struct FSOUND_STREAM {
    Mix_Music* music;
    int loops;
};
struct FSOUND_SAMPLE {
    Mix_Chunk* chunk;
    int loop;
};

typedef struct FMUSIC_MODULE FMUSIC_MODULE;
typedef struct FSOUND_STREAM FSOUND_STREAM;
typedef struct FSOUND_SAMPLE FSOUND_SAMPLE;

FSOUND_SAMPLE* FSOUND_Sample_Load(int index,
                                  const char* buffer,
                                  unsigned int mode,
                                  int memlength) {
    if (!buffer) return nullptr;

    FSOUND_SAMPLE* sample = new FSOUND_SAMPLE{};
    sample->loop = 0;
    if ((mode & FSOUND_LOADMEMORY) != 0) {
        if (memlength <= 0) {
            debug << "FSOUND_Sample_Load: invalid memory length " << memlength
                  << "\n";
            delete sample;
            return nullptr;
        }
        sample->chunk =
            Mix_LoadWAV_RW(SDL_RWFromConstMem(buffer, memlength), 1);
    } else {
        sample->chunk = Mix_LoadWAV(buffer);
    }
    if (!sample->chunk) {
        debug << "FSOUND_Sample_Load: " << Mix_GetError() << "\n";
        delete sample;
        return nullptr;
    }

    return sample;
}
int FSOUND_PlaySound(int channel, FSOUND_SAMPLE* sptr) {
    if (!sptr || !sptr->chunk) return -1;
    const int result = Mix_PlayChannel(channel, sptr->chunk, sptr->loop);
    if (result < 0) {
        debug << "Mix_PlayChannel: " << Mix_GetError() << "\n";
    } else if (!logged_sfx_playback) {
        debug << "Sound-effect playback started on channel " << result << "\n";
        logged_sfx_playback = true;
    }
    return result;
}
signed char FSOUND_Sample_SetLoopMode(FSOUND_SAMPLE* sptr,
                                      unsigned int loopmode) {
    if (!sptr) return false;
    sptr->loop = (loopmode & FSOUND_LOOP_NORMAL) != 0 ? -1 : 0;
    return true;
}

signed char FSOUND_StopSound(int channel) {
    Mix_HaltChannel(channel);
    return true;
}

void FSOUND_Sample_Free(FSOUND_SAMPLE* sptr) {
    if (!sptr) return;
    const int channel_count = Mix_AllocateChannels(-1);
    for (int channel = 0; channel < channel_count; ++channel) {
        if (Mix_GetChunk(channel) == sptr->chunk) Mix_HaltChannel(channel);
    }
    Mix_FreeChunk(sptr->chunk);
    delete sptr;
}

FSOUND_STREAM* FSOUND_Stream_OpenFile(const char* filename,
                                      unsigned int mode,
                                      int memlength) {
    if (!filename) return nullptr;
    FSOUND_STREAM* stream = new FSOUND_STREAM{};
    stream->music = LoadMusic(filename);
    if (!stream->music) {
        debug << "Mix_LoadMUS(\"" << filename << "\"): " << Mix_GetError()
              << "\n";
        delete stream;
        return nullptr;
    }
    stream->loops = (mode & FSOUND_LOOP_NORMAL) != 0 ? -1 : 0;
    return stream;
}
int FSOUND_Stream_Play(int channel, FSOUND_STREAM* stream) {
    if (!stream || !stream->music) return false;
    if (Mix_PlayMusic(stream->music, stream->loops) != 0) {
        debug << "Mix_PlayMusic: " << Mix_GetError() << "\n";
        return false;
    }
    debug << "Music playback started (loops=" << stream->loops << ")\n";
    return true;
}
signed char FSOUND_Stream_Stop(FSOUND_STREAM* stream) {
    if (!stream) return false;
    Mix_HaltMusic();
    return true;
}
signed char FSOUND_Stream_Close(FSOUND_STREAM* stream) {
    if (!stream) return true;
    if (Mix_PlayingMusic()) Mix_HaltMusic();
    Mix_FreeMusic(stream->music);
    delete stream;
    return true;
}

signed char FSOUND_Init(int mixrate,
                        int maxsoftwarechannels,
                        unsigned int flags) {
    const int required_codecs = MIX_INIT_MP3 | MIX_INIT_OGG;
    const int initialized_codecs = Mix_Init(required_codecs);
    for (const int codec : {MIX_INIT_MP3, MIX_INIT_OGG}) {
        debug << "SDL_mixer " << CodecName(codec) << " codec: "
              << ((initialized_codecs & codec) != 0 ? "available" : "missing")
              << "\n";
    }
    if ((initialized_codecs & required_codecs) != required_codecs) {
        debug << "Mix_Init failed to initialize required codecs: "
              << Mix_GetError() << "\n";
        while (Mix_Init(0)) Mix_Quit();
        return 0;
    }

    if (Mix_OpenAudio(mixrate, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
        debug << "Mix_OpenAudio: " << Mix_GetError() << "\n";
        while (Mix_Init(0)) Mix_Quit();
        return 0;
    }
    mixer_open = true;
    logged_sfx_playback = false;
    Mix_AllocateChannels(maxsoftwarechannels);

    int actual_rate = 0;
    Uint16 actual_format = 0;
    int actual_channels = 0;
    if (Mix_QuerySpec(&actual_rate, &actual_format, &actual_channels)) {
        debug << "SDL_mixer audio opened: " << actual_rate << " Hz, "
              << actual_channels << " channels, format 0x" << std::hex
              << actual_format << std::dec << "\n";
    }
    return 1;
}
void FSOUND_Close() {
    if (mixer_open) {
        Mix_HaltMusic();
        Mix_HaltChannel(-1);
        Mix_CloseAudio();
        mixer_open = false;
    }
    while (Mix_Init(0)) Mix_Quit();
}
int FSOUND_GetError() { return true; }
signed char FMUSIC_PlaySong(FMUSIC_MODULE* mod) { return true; }
FMUSIC_MODULE* FMUSIC_LoadSong(const char* name) { return nullptr; }
signed char FMUSIC_SetMasterVolume(FMUSIC_MODULE* mod, int volume) {
    return true;
}
signed char FMUSIC_StopSong(FMUSIC_MODULE* mod) { return true; }
signed char FMUSIC_FreeSong(FMUSIC_MODULE* mod) { return true; }
signed char FSOUND_SetPriority(int channel, int priority) { return true; }
}
