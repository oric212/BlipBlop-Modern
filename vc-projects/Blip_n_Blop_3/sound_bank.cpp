#include "sound_bank.h"

#include <fstream>
#include <vector>

#include "ben_debug.h"

void SoundBank::reload() {
    if (!filename_.empty()) loadSFX(filename_.c_str());
}

bool SoundBank::loadSFX(const char* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        debug << "SoundBank::loadSFX: cannot open " << filename << "\n";
        return false;
    }

    int sound_count = 0;
    file.read(reinterpret_cast<char*>(&sound_count), sizeof(sound_count));
    if (!file || sound_count < 1) {
        debug << "SoundBank::loadSFX: invalid sound count in " << filename
              << "\n";
        return false;
    }

    std::vector<std::unique_ptr<Sound>> loaded_sounds;
    loaded_sounds.reserve(static_cast<size_t>(sound_count));
    for (int index = 0; index < sound_count; ++index) {
        int buffer_count = 0;
        int byte_count = 0;
        file.read(reinterpret_cast<char*>(&buffer_count), sizeof(buffer_count));
        file.read(reinterpret_cast<char*>(&byte_count), sizeof(byte_count));
        (void)buffer_count;
        if (!file || byte_count <= 0) {
            debug << "SoundBank::loadSFX: invalid entry " << index << " in "
                  << filename << "\n";
            return false;
        }

        std::vector<char> bytes(static_cast<size_t>(byte_count));
        file.read(bytes.data(), byte_count);
        if (!file) {
            debug << "SoundBank::loadSFX: truncated entry " << index << " in "
                  << filename << "\n";
            return false;
        }

        auto sound = std::make_unique<Sound>();
        if (!sound->loadFromMem(bytes.data(), byte_count)) {
            debug << "SoundBank::loadSFX: cannot decode sound " << index
                  << " in " << filename << "\n";
            return false;
        }
        loaded_sounds.push_back(std::move(sound));
    }

    tab_ = std::move(loaded_sounds);
    filename_ = filename;
    debug << "Loaded " << tab_.size() << " sound effect(s) from " << filename
          << "\n";
    return true;
}
