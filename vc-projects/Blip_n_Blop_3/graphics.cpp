#include "graphics.h"

#include <algorithm>

#include "errors.h"

extern SDL::Surface* backSurface;

namespace {

constexpr int kLogicalWidth = 640;
constexpr int kLogicalHeight = 480;

}  // namespace

void Graphics::Init() {
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        throw std::runtime_error(std::string("Can't initialize SDL") +
                                 SDL_GetError());
    }
}

void Graphics::ToggleFullscreen() {
    if (!window_) return;

    if (!fullscreen_) {
        SDL_GetWindowPosition(window_.get(), &windowed_x_, &windowed_y_);
        SDL_GetWindowSize(window_.get(), &windowed_width_, &windowed_height_);
    }

    const Uint32 mode = fullscreen_ ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (SDL_SetWindowFullscreen(window_.get(), mode) != 0) {
        debug << "Cannot toggle fullscreen: " << SDL_GetError() << "\n";
        return;
    }

    fullscreen_ = !fullscreen_;
    if (!fullscreen_) {
        SDL_SetWindowSize(window_.get(), windowed_width_, windowed_height_);
        SDL_SetWindowPosition(window_.get(), windowed_x_, windowed_y_);
    }
}

void Graphics::SetGfxMode(int x, int y, int d, bool fullscreen) {
    x_ = x;
    y_ = y;
    d_ = d;
    fullscreen_ = fullscreen;

    const Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_ALLOW_HIGHDPI |
                                (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    window_.reset(SDL_ErrWrap(SDL_CreateWindow(
        "Blip&Blop",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        x,
        y,
        window_flags)));

    renderer_.reset(SDL_ErrWrap(SDL_CreateRenderer(
        window_.get(),
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)));

    SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
    last_output_width_ = 0;
    last_output_height_ = 0;
}

void Graphics::CreateFrameTexture() {
    if (frame_texture_ || !renderer_ || !backSurface || !backSurface->Get()) {
        return;
    }

    SDL_Surface* const frame = backSurface->Get();
    frame_texture_.reset(SDL_ErrWrap(SDL_CreateTexture(renderer_.get(),
                                                       frame->format->format,
                                                       SDL_TEXTUREACCESS_STREAMING,
                                                       kLogicalWidth,
                                                       kLogicalHeight)));
    SDL_SetTextureScaleMode(frame_texture_.get(), SDL_ScaleModeNearest);
}

SDL_Rect Graphics::PresentationRect(int output_width, int output_height) const {
    int width;
    int height;
    if (static_cast<int64_t>(output_width) * kLogicalHeight <=
        static_cast<int64_t>(output_height) * kLogicalWidth) {
        width = output_width;
        height = std::max(1, output_width * kLogicalHeight / kLogicalWidth);
    } else {
        height = output_height;
        width = std::max(1, output_height * kLogicalWidth / kLogicalHeight);
    }
    return {(output_width - width) / 2,
            (output_height - height) / 2,
            width,
            height};
}

SDL::Surface* Graphics::CreatePrimary() {
    /**/
    debug << "CreatePrimary() - Creating a 640 x 480 Surface"
          << "\n";
    return CreateSurface(640, 480, 0);
    // return 0;
}

SDL::Surface* Graphics::CreatePrimary(SDL::Surface*& back) {
    debug << "Graphics::CreatePrimary(SDL::Surface * & back) - Creating a "
             "640x480 surface"
          << "\n";
    SDL::Surface* tmp = CreateSurface(640, 480);
    back = CreateSurface(640, 480);
    tmp->SetBackBuffer(back);
    return tmp;
    // return 0;
}

SDL::Surface* Graphics::CreateSurface(int x, int y) {
    return CreateSurface(x, y, 0);
}

SDL::Surface* Graphics::CreateSurface(int x, int y, int flags) {
    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    SDL_Surface* surf =
        SDL_CreateRGBSurface(0, x, y, 32, rmask, gmask, bmask, amask);

    SDL::Surface* tmp = new SDL::Surface(surf);
    tmp->FillRect(0, 0xFF000000);
    return tmp;
}

SDL_Surface* Graphics::CreateSDLSurface(int x, int y) {
    Uint32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    SDL_Surface* surf =
        SDL_CreateRGBSurface(0, x, y, 32, rmask, gmask, bmask, amask);
    return (surf);
}

SDL::Surface* Graphics::LoadBMP(char* file) { return this->LoadBMP(file, 0); }

SDL::Surface* Graphics::LoadBMP(char* file, int flags) {
    /*SDL_Surface *bmp = 0;
      bmp = SDL_LoadBMP(file);
      if (bmp == 0){
      std::cout << SDL_GetError() << std::endl;
      return 0;
      }
      SDL_Texture *tex = 0;
      tex = SDL_CreateTextureFromSurface(ren, bmp);
      SDL_FreeSurface(bmp);
      return new SDL::Surface(tex);*/
    SDL_Surface* bmp = 0;
    bmp = SDL_LoadBMP(file);
    if (bmp == 0) {
        std::cout << SDL_GetError() << std::endl;
        return 0;
    }
    return new SDL::Surface(bmp);
}

bool Graphics::SetColorKey(SDL::Surface* surf, Pixel rgb) {
    SDL_SetColorKey(surf->Get(),
                    SDL_TRUE,
                    SDL_MapRGB(surf->Get()->format,
                               (rgb & 0xFF),
                               ((rgb >> 8) & 0xFF),
                               ((rgb >> 16) & 0xFF)));
    // TODO: set color key
    return true;
}

void Graphics::Flip() {
    if (!renderer_ || !backSurface || !backSurface->Get()) return;

    CreateFrameTexture();
    if (!frame_texture_) return;

    SDL_Surface* const frame = backSurface->Get();
    if (SDL_UpdateTexture(frame_texture_.get(), NULL, frame->pixels, frame->pitch) !=
        0) {
        debug << "Cannot update frame texture: " << SDL_GetError() << "\n";
        return;
    }

    int output_width = 0;
    int output_height = 0;
    if (SDL_GetRendererOutputSize(renderer_.get(), &output_width, &output_height) !=
        0 ||
        output_width <= 0 || output_height <= 0) {
        debug << "Cannot query renderer output size: " << SDL_GetError() << "\n";
        return;
    }

    const SDL_Rect destination = PresentationRect(output_width, output_height);
    SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
    SDL_RenderClear(renderer_.get());
    SDL_RenderCopy(renderer_.get(), frame_texture_.get(), NULL, &destination);
    SDL_RenderPresent(renderer_.get());

    if (output_width != last_output_width_ || output_height != last_output_height_) {
        debug << "Presentation: drawable " << output_width << "x" << output_height
              << ", game " << destination.w << "x" << destination.h << " at "
              << destination.x << "," << destination.y << "\n"
              << std::flush;
        last_output_width_ = output_width;
        last_output_height_ = output_height;
    }
}

void Graphics::FlipV() {
    Flip();
    // SDL_RenderPresent(renderer);
}

void Graphics::Clear(int r, int g, int b) {
    SDL_SetRenderDrawColor(renderer_.get(), r, g, b, 255);
    SDL_RenderClear(renderer_.get());
    SDL_RenderPresent(renderer_.get());
}

void Graphics::Clear(int c) {
    int r = (c >> 16) & 0xFF;
    int g = (c >> 8) & 0xFF;
    int b = (c >> 0) & 0xFF;

    SDL_SetRenderDrawColor(renderer_.get(), r, g, b, 255);
    SDL_RenderClear(renderer_.get());
    SDL_RenderPresent(renderer_.get());
}

void Graphics::Clear(RenderRect r2) {
    int r = (r2.dwFillColor >> 16) & 0xFF;
    int g = (r2.dwFillColor >> 8) & 0xFF;
    int b = (r2.dwFillColor >> 0) & 0xFF;

    SDL_Rect rect;
    rect.x = r2.left;
    rect.y = r2.top;
    rect.w = r2.right - r2.left;
    rect.h = r2.bottom - r2.top;

    SDL_SetRenderDrawColor(renderer_.get(), r, g, b, 255);
    SDL_RenderDrawRect(renderer_.get(), &rect);
}
