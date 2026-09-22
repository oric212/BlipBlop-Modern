#include "music_bank.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include "ben_debug.h"
#include "config.h"
#include "fmod__errors.h"
#include "runtime_paths.h"

#define TYPE_MOD 0
#define TYPE_MP3 1

bool MusicBank::open(const char* file, bool loop) {
    if (!music_on) return true;

    std::ifstream f(file, std::ios::in);

    if (f.is_open() == 0) {
        debug << "MusicBank::load() -> Impossible d'ouvrir le fichier " << file
              << "\n";
        return false;
    }

    int nb_musics;
    f >> nb_musics;

    if (nb_musics < 1) {
        f.close();
        debug << "MusicBank::load() -> Fichier " << file << " corrompu ("
              << nb_musics << ")\n";
        return false;
    }

    std::vector<std::unique_ptr<Music>> loaded_musics;
    loaded_musics.reserve(nb_musics);

    for (int i = 0; i < nb_musics; i++) {
        int type;
        f >> type;
        std::string fname;
        f >> fname;
        if (!f) {
            debug << "MusicBank::load() -> Fichier " << file
                  << " tronque a l'entree " << i << "\n";
            return false;
        }
        fname = RuntimePaths::resolveString(fname);

        try {
            if (type == TYPE_MOD) {
                loaded_musics.push_back(std::make_unique<ModMusic>(fname));
            } else if (type == TYPE_MP3) {
                loaded_musics.push_back(
                    std::make_unique<Mp3Music>(fname, loop));
            } else {
                debug << "MusicBank::load() -> Type de musique inconnu "
                      << type << " dans " << file << "\n";
                return false;
            }
        } catch (const std::exception& error) {
            debug << "MusicBank::load() -> " << error.what() << " pour "
                  << fname << "\n";
            return false;
        }
    }

    musics_ = std::move(loaded_musics);
    debug << "Loaded " << musics_.size() << " music(s) from " << file
          << "\n";
    return true;
}

void MusicBank::play(int n) {
    if (!music_on) return;

    if (n < 0 || static_cast<size_t>(n) >= musics_.size()) {
        debug << "MusicBank::play() -> Tentative de jouer une musique non "
                 "chargée : "
              << n << "\n";
        return;
    }

    if (!musics_[n]) return;
    musics_[n]->Play();
}

void MusicBank::stop(int n) {
    if (!music_on) return;

    if (n < 0 || static_cast<size_t>(n) >= musics_.size()) {
        debug << "MusicBank::stop() -> Tentative de stoper une musique non "
                 "chargée : "
              << n << "\n";
        return;
    }

    if (musics_[n]) musics_[n]->Stop();
}

void MusicBank::stop() {
    for (const auto& music : musics_) {
        if (music) music->Stop();
    }
}

void MusicBank::setVol(int v) {
    if (!music_on) return;

    for (const auto& music : musics_) {
        if (music) music->set_volume(v);
    }
}
