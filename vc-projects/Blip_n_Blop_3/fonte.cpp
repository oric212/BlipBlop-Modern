/******************************************************************
*
*
*		----------------
*		  Fonte.h
*		----------------
*
*		Classe Fonte
*
*		Affiche du joli texte
*
*
*		Prosper / LOADED -   V 0.2
*
*
*
******************************************************************/

//-----------------------------------------------------------------------------
//		Headers
//-----------------------------------------------------------------------------

#include <fstream>

#include "graphics.h"
#include <string.h>
#include <fstream>
#include <fcntl.h>
#include <stdio.h>
#include <vector>

#include "lgx_packer.h"
#include "ben_debug.h"
#include "fonte.h"


//-----------------------------------------------------------------------------

bool Fonte::load(const char * fic, int flags)
{
        std::ifstream fh(fic, std::ios::binary | std::ios::ate);

	if (!fh.good()) {
		debug << "Fonte::load->Ne peut pas ouvrir " << fic << "\n";
		return false;
	}
	const std::streamoff file_size = fh.tellg();
	if (file_size < static_cast<std::streamoff>(2 * sizeof(int) + 255 * sizeof(int)))
		return false;
	fh.seekg(0);
	int loaded_h = 0;
	int loaded_spc = 0;
	if (!fh.read(reinterpret_cast<char*>(&loaded_h), sizeof(loaded_h)) ||
	    !fh.read(reinterpret_cast<char*>(&loaded_spc), sizeof(loaded_spc)))
		return false;
	std::vector<std::unique_ptr<Picture>> loaded(256);
        for (int i = 1; i < 256; i++) {
            int taille = 0;
            if (!fh.read(reinterpret_cast<char*>(&taille), sizeof(taille)) || taille < 0)
                return false;

            if (taille == 0) {
                continue;
            }

			if (taille < static_cast<int>(sizeof(LGX_HEADER)) ||
			    static_cast<std::streamoff>(taille) > file_size - fh.tellg()) return false;
			std::vector<char> bytes(static_cast<size_t>(taille));
			if (!fh.read(bytes.data(), taille)) return false;
			SDL::Surface* surf = LGXpaker.loadLGX(bytes.data(), bytes.size(), flags);
			if (!surf) return false;
			loaded[i] = std::make_unique<Picture>();
			loaded[i]->SetSurface(surf);
			loaded[i]->SetSpot(0, 0);
            //		pictab[i]->SetColorKey( RGB( 250, 212, 152));
            loaded[i]->SetColorKey(RGB( 246, 205, 148));
            //pictab[i]->SetColorKey(RGB(250, 206, 152));

            /*static int test_i = 1;
              char buf[128];
              sprintf(buf, "test/%d.bmp", test_i);
              SDL_SaveBMP(surf->Get(), buf);
              test_i++;*/

        }

        pictab_ = std::move(loaded);
        h = loaded_h;
        spc = loaded_spc;
        filename_ = fic;
        flag_fic = flags;
        return true;
}

//-----------------------------------------------------------------------------

void Fonte::print(SDL::Surface * surf, int x, int y, const char * txt)
{
	if (txt == NULL)
		return;

	int		curx;	// X courant
	int		c;

	curx = x;

	for (unsigned int i = 0; i < strlen(txt); i++) {
		c = (unsigned char) txt[i];
		if (c == ' ') {
			curx += spc;
		} else if (pictab_[c] != NULL) {
			pictab_[c]->BlitTo(surf, curx, y);
			curx += pictab_[c]->xSize();
		}
	}
}


//-----------------------------------------------------------------------------

void Fonte::printM(SDL::Surface * surf, int x, int y, const char * txt, int ym)
{
	if (txt == NULL)
		return;

	int		curx;	// X courant
	int		cury;	// Y courant
	int		nx;		// Next x (pour ne pas le calculer 2 fois)
	unsigned int		c;

	curx = x;
	cury = y;


	for (unsigned int i = 0; i < strlen(txt); i++) {
		c = (unsigned char) txt[i];

		if (c == ' ') {
			curx += spc;
		} else if (pictab_[c] != NULL) {
			// Calcul du X suivant
			nx = curx + pictab_[c]->xSize();

			// Ligne suivante ?
			if (nx >= ym) {
				cury += h;
				pictab_[c]->BlitTo(surf, x, cury);
				curx = x + pictab_[c]->xSize();
			} else {	// Non, on continue..
				pictab_[c]->BlitTo(surf, curx, cury);
				curx = nx;
			}
		}
	}
}


//-----------------------------------------------------------------------------

void Fonte::printR(SDL::Surface * surf, int x, int y, const char * txt)
{
	if (txt == NULL)
		return;

	int		l = 0;	// Longueur en pixels de la chaîne
	int		c;

	for (unsigned int i = 0; i < strlen(txt); i++) {
		c = (unsigned char) txt[i];

		if (c == ' ')
			l += spc;
		else if (pictab_[c] != NULL)
			l += pictab_[c]->xSize();
	}

	print(surf, x - l, y, txt);
}


//-----------------------------------------------------------------------------

void Fonte::printC(SDL::Surface * surf, int xtaille, int y, const char * txt)
{
	if (txt == NULL)
		return;

	int		l = 0;	// Longueur en pixels de la chaîne
	int		c;

	for (unsigned int i = 0; i < strlen(txt); i++) {
		c = (unsigned char) txt[i];

		if (c == ' ')
			l += spc;
		else if (pictab_[c] != NULL)
			l += pictab_[c]->xSize();
	}

	print(surf, xtaille - (l >> 1), y, txt);
}

//-----------------------------------------------------------------------------

void Fonte::printMW(SDL::Surface * surf, int x, int y, const char * srctxt, int ym)
{
	static const char delim [] = " ";

	if (srctxt == NULL)
		return;

	int		curx;	// X courant
	int		cury;	// Y courant
	int		nx;		// Next x (pour ne pas le calculer 2 fois)

	curx = x;
	cury = y;

	std::vector<char> txt(strlen(srctxt) + 1);
	char * token;

	strcpy(txt.data(), srctxt);
	token = strtok(txt.data(), delim);

	while (token != NULL) {
		nx = width(token);

		if (curx + nx > ym) {
			cury += h;
			print(surf, x, cury, token);
			curx = x + nx;
		} else {
			print(surf, curx, cury, token);
			curx += nx;
		}

		curx += spc;

		token = strtok(NULL, delim);
	}
}


//-----------------------------------------------------------------------------

int Fonte::width(const char * txt)
{
	if (txt == NULL)
		return 0;

	int		l = 0;	// Longueur en pixels de la chaîne
	int		c;

	for (unsigned int i = 0; i < strlen(txt); i++) {
		c = (unsigned char) txt[i];

		if (c == ' ')
			l += spc;
		else if (pictab_[c] != NULL)
			l += pictab_[c]->xSize();
	}

	return l;
}

//-----------------------------------------------------------------------------

bool Fonte::restoreAll()
{
	if (filename_.empty())
		return true;
	const std::string file = filename_;
	if (!load(file.c_str(), flag_fic)) return false;
	for (auto& picture : pictab_)
		if (picture) picture->SetColorKey(RGB(250, 206, 152));
	return true;
}
