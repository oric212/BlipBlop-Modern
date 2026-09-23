#include "picture_bank.h"

#include <fstream>
#include <vector>

#include "ben_debug.h"
#include "dd_gfx.h"
#include "lgx_packer.h"

namespace {

bool readInt(std::ifstream& file, int& value) {
    return static_cast<bool>(
        file.read(reinterpret_cast<char*>(&value), sizeof(value)));
}

std::streamoff remainingBytes(std::ifstream& file) {
    const std::streampos current = file.tellg();
    if (current < 0) return -1;
    file.seekg(0, std::ios::end);
    const std::streampos end = file.tellg();
    file.seekg(current);
    return end >= current ? end - current : -1;
}

bool readEntry(std::ifstream& file, int& xspot, int& yspot,
               std::vector<char>& bytes) {
    int size = 0;
    if (!readInt(file, xspot) || !readInt(file, yspot) ||
        !readInt(file, size) || size <= 0) {
        return false;
    }
    const std::streamoff remaining = remainingBytes(file);
    if (remaining < 0 || static_cast<std::streamoff>(size) > remaining)
        return false;

    bytes.resize(static_cast<size_t>(size));
    return static_cast<bool>(file.read(bytes.data(), size));
}

}  // namespace

PictureBank::PictureBank() : flag_fic(0), trans_fic(false) {}

bool PictureBank::loadGFX(const char* file, int flag, bool trans) {
    std::ifstream input(file, std::ios::binary);
    if (!input) {
        debug << "PictureBank::loadGFX() - cannot open " << file << "\n";
        return false;
    }

    int picture_count = 0;
    if (!readInt(input, picture_count) || picture_count < 0) {
        debug << "PictureBank::loadGFX() - invalid picture count in " << file
              << "\n";
        return false;
    }
    const std::streamoff remaining = remainingBytes(input);
    if (remaining < 0 || static_cast<std::streamoff>(picture_count) >
                             remaining / (3 * sizeof(int))) {
        debug << "PictureBank::loadGFX() - impossible picture count in "
              << file << "\n";
        return false;
    }

    std::vector<std::unique_ptr<Picture>> loaded;
    loaded.reserve(static_cast<size_t>(picture_count));
    for (int index = 0; index < picture_count; ++index) {
        int xspot = 0;
        int yspot = 0;
        int version = 0;
        std::vector<char> bytes;
        if (!readEntry(input, xspot, yspot, bytes)) {
            debug << "PictureBank::loadGFX() - invalid/truncated entry "
                  << index << " in " << file << "\n";
            return false;
        }

        SDL::Surface* surface = LGXpaker.loadLGX(bytes.data(), bytes.size(), flag, &version);
        if (surface == nullptr) {
            debug << "PictureBank::loadGFX() - cannot decode entry " << index
                  << " in " << file << "\n";
            return false;
        }

        auto picture = std::make_unique<Picture>();
        picture->SetSpot(xspot, yspot);
        picture->SetSurface(surface);
        if (trans) {
            picture->SetColorKey(version == 1 ? RGB(246, 205, 148)
                                               : RGB(246, 210, 148));
        }
        loaded.push_back(std::move(picture));
    }

    tab_ = std::move(loaded);
    filename_ = file;
    flag_fic = flag;
    trans_fic = trans;
    return true;
}

bool PictureBank::restoreAll() {
    if (filename_.empty()) return true;

    std::ifstream input(filename_, std::ios::binary);
    if (!input) {
        debug << "PictureBank::restoreAll() - cannot open " << filename_
              << "\n";
        return false;
    }

    int picture_count = 0;
    if (!readInt(input, picture_count) || picture_count < 0 ||
        static_cast<size_t>(picture_count) != tab_.size()) {
        debug << "PictureBank::restoreAll() - invalid picture count in "
              << filename_ << "\n";
        return false;
    }

    auto release_surface = [](SDL::Surface* surface) {
        if (surface) surface->Release();
    };
    using PendingSurface = std::unique_ptr<SDL::Surface, decltype(release_surface)>;
    std::vector<PendingSurface> decoded;
    std::vector<int> versions;
    std::vector<std::pair<int, int>> spots;
    decoded.reserve(static_cast<size_t>(picture_count));
    versions.reserve(static_cast<size_t>(picture_count));
    spots.reserve(static_cast<size_t>(picture_count));
    for (int index = 0; index < picture_count; ++index) {
        int xspot = 0;
        int yspot = 0;
        int version = 0;
        std::vector<char> bytes;
        if (!readEntry(input, xspot, yspot, bytes)) {
            debug << "PictureBank::restoreAll() - invalid/truncated entry "
                  << index << " in " << filename_ << "\n";
            return false;
        }

        SDL::Surface* surface =
            LGXpaker.loadLGX(bytes.data(), bytes.size(), flag_fic, &version);
        if (surface == nullptr) {
            debug << "PictureBank::restoreAll() - cannot decode entry "
                  << index << " in " << filename_ << "\n";
            return false;
        }

        decoded.emplace_back(surface, release_surface);
        versions.push_back(version);
        spots.emplace_back(xspot, yspot);
    }
    for (int index = 0; index < picture_count; ++index) {
        if (!tab_[index]) return false;
    }
    for (int index = 0; index < picture_count; ++index) {
        SDL::Surface* old_surface = tab_[index]->Surf();
        if (old_surface) old_surface->Release();
        tab_[index]->SetSpot(spots[index].first, spots[index].second);
        tab_[index]->SetSurface(decoded[index].release());
        if (trans_fic)
            tab_[index]->SetColorKey(versions[index] == 1 ? RGB(250, 206, 152)
                                                         : RGB(250, 214, 152));
    }
    return true;
}
