/******************************************************************
*
*
*		--------------
*		  Config.cpp
*		--------------
*
*		Contient toutes les données sur la config
*		et quelques fonctions pour la gérer.
*
*
*		Prosper / LOADED -   V 0.2
*
*
*
******************************************************************/

#include <stdio.h>
#include "dd_gfx.h"
#include "ben_debug.h"
#include "config.h"
#include "input.h"
#include "control_alias.h"
#include "fmod.h"
#include "globals.h"
#include "runtime_paths.h"

#include <array>
#include <filesystem>

namespace {

constexpr std::array<int, 14> aliases = {
    ALIAS_P1_UP,   ALIAS_P1_DOWN, ALIAS_P1_LEFT, ALIAS_P1_RIGHT,
    ALIAS_P1_FIRE, ALIAS_P1_JUMP, ALIAS_P1_SUPER,
    ALIAS_P2_UP,   ALIAS_P2_DOWN, ALIAS_P2_LEFT, ALIAS_P2_RIGHT,
    ALIAS_P2_FIRE, ALIAS_P2_JUMP, ALIAS_P2_SUPER};

template <typename T>
bool readValue(FILE* file, T& value) {
    return fread(&value, sizeof(value), 1, file) == 1;
}

template <typename T>
bool writeValue(FILE* file, const T& value) {
    return fwrite(&value, sizeof(value), 1, file) == 1;
}

}  // namespace

bool	vSyncOn = true;

int		mem_flag = DDSURF_BEST;
bool	video_buffer_on = true;
bool	mustFixGforceBug = false;

int		lang_type = LANG_UK;

bool	music_on = true;
bool	sound_on = true;

bool	cheat_on = false;

HiScores	hi_scores;

bool	winSet;
bool fullscreen = true; // THIS IS UGLY AS FUCK. WAY TOO MANY GLOBALS


void load_BB3_config(const char * cfg_file)
{
	set_default_config(true);
	FILE *fic = fopen(cfg_file, "rb");

	if (fic == NULL) {
		debug << "Cannot find config file. Will use default config.\n";
	} else {
		fseek(fic, 0, SEEK_END);
		const long file_size = ftell(fic);
		rewind(fic);
		bool loaded_vsync = true;
		bool loaded_fullscreen = fullscreen;
		int loaded_language = LANG_UK;
		std::array<int, aliases.size()> loaded_aliases{};
		const long legacy_size = sizeof(loaded_vsync) + sizeof(loaded_language) +
		                         sizeof(int) * loaded_aliases.size();
		const long current_size = legacy_size + sizeof(loaded_fullscreen);
		bool valid = file_size == legacy_size || file_size == current_size;
		valid = valid && readValue(fic, loaded_vsync);
		if (file_size == current_size)
			valid = valid && readValue(fic, loaded_fullscreen);
		valid = valid && readValue(fic, loaded_language);
		for (int& value : loaded_aliases) valid = valid && readValue(fic, value);
		valid = valid && fgetc(fic) == EOF &&
		        (loaded_language == LANG_FR || loaded_language == LANG_UK);
		fclose(fic);

		if (!valid) {
			debug << "Configuration file is malformed or truncated. Using defaults: "
			      << cfg_file << "\n";
		} else {
			vSyncOn = loaded_vsync;
			fullscreen = loaded_fullscreen;
			lang_type = loaded_language;
			for (size_t i = 0; i < aliases.size(); ++i)
				in.setAlias(aliases[i], loaded_aliases[i]);
			debug << "Using " << cfg_file << " as configuration file.\n";
		}
	}

	lang_type = LANG_UK;
}

void save_BB3_config(const char * cfg_file)
{
	const std::filesystem::path destination(cfg_file);
	const std::filesystem::path temporary = destination.string() + ".tmp";
	FILE *fic = fopen(temporary.string().c_str(), "wb");

	if (fic == NULL) {
		debug << "Cannot save config file.\n";
	} else {
		debug << "Saving " << cfg_file << " as configuration file.\n";
		bool valid = writeValue(fic, vSyncOn) && writeValue(fic, fullscreen) &&
		             writeValue(fic, lang_type);
		for (int alias : aliases) {
			const int value = static_cast<int>(in.getAlias(alias));
			valid = valid && writeValue(fic, value);
		}
		const bool flushed = fflush(fic) == 0;
		const bool closed = fclose(fic) == 0;
		valid = valid && flushed && closed;
		if (!valid) {
			debug << "Cannot complete config write; keeping previous file.\n";
			std::error_code ec;
			std::filesystem::remove(temporary, ec);
			return;
		}
		std::string error;
		if (!RuntimePaths::commitTemporaryFile(temporary, destination, error)) {
			debug << "Cannot replace config file: " << error << "\n";
			std::error_code ec;
			std::filesystem::remove(temporary, ec);
		}
	}
}

void set_default_config(bool reset_lang)
{
	fullscreen = true;
	if (reset_lang)
		lang_type = LANG_UK;

	in.setAlias(ALIAS_P1_UP, DIK_UP);
	in.setAlias(ALIAS_P1_DOWN, DIK_DOWN);
	in.setAlias(ALIAS_P1_LEFT, DIK_LEFT);
	in.setAlias(ALIAS_P1_RIGHT, DIK_RIGHT);
	in.setAlias(ALIAS_P1_FIRE, DIK_LCONTROL);
	in.setAlias(ALIAS_P1_JUMP, DIK_LMENU);
	in.setAlias(ALIAS_P1_SUPER, DIK_SPACE);

	in.setAlias(ALIAS_P2_UP, DIK_Q);
	in.setAlias(ALIAS_P2_DOWN, DIK_S);
	in.setAlias(ALIAS_P2_LEFT, DIK_D);
	in.setAlias(ALIAS_P2_RIGHT, DIK_F);
	in.setAlias(ALIAS_P2_FIRE, DIK_TAB);
	in.setAlias(ALIAS_P2_JUMP, DIK_G);
	in.setAlias(ALIAS_P2_SUPER, DIK_H);
}
