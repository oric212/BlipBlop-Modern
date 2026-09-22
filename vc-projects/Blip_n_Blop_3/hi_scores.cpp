
#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <filesystem>
#include "hi_scores.h"
#include "runtime_paths.h"

namespace {

bool readBytes(FILE* file, void* data, size_t size) {
	return fread(data, 1, size, file) == size;
}

bool writeBytes(FILE* file, const void* data, size_t size) {
	return fwrite(data, 1, size, file) == size;
}

}  // namespace

void HiScores::init()
{
	int	scr = 10000 * HS_NB_SCORES;

	for (int i = 0; i < HS_NB_SCORES; i++) {
		scores[i] = scr;
		memset(names[i], 0, HS_NAME_LENGTH);
		strcpy(names[i], "LOADED STUDIO");
		scr -= 10000;
	}
}

void HiScores::add(int scr, const char * name)
{
	int		i = 0;

	while (i < HS_NB_SCORES && scores[i] > scr)
		i++;

	int		j = HS_NB_SCORES - 1;

	while (j > i) {
		scores[j] = scores[j - 1];
		strcpy(names[j], names[j - 1]);

		j--;
	}

	if (i < HS_NB_SCORES) {
		scores[i] = scr;
		strcpy(names[i], name);
	}
}

void HiScores::crypte()
{
	for (int i = 0; i < HS_NB_SCORES; i++) {
		const std::uint32_t mask = 0x35674a1fU << i;
		scores[i] = static_cast<int>(
		    static_cast<std::uint32_t>(scores[i]) ^ mask);
		for (int j = 0; j < HS_NAME_LENGTH; j += 4) {
			std::uint32_t word = 0;
			memcpy(&word, &names[i][j], sizeof(word));
			word ^= mask;
			memcpy(&names[i][j], &word, sizeof(word));
		}
	}
}

bool HiScores::save(const char * file)
{
	const std::filesystem::path destination(file);
	const std::filesystem::path temporary = destination.string() + ".tmp";
	FILE *f = fopen(temporary.string().c_str(), "wb");

	if (f == NULL)
		return false;

	std::int64_t sum = 0;

	for (int i = 0; i < HS_NB_SCORES; i++) {
		sum += scores[i];

		for (int j = 0; j < HS_NAME_LENGTH; j++)
			sum += names[i][j];
	}
	const int som = static_cast<int>(sum);

	crypte();

	bool valid = true;
	for (int i = 0; i < HS_NB_SCORES; i++) {
		valid = valid && writeBytes(f, &scores[i], sizeof(scores[i]));
		valid = valid && writeBytes(f, names[i], HS_NAME_LENGTH);
	}
	valid = valid && writeBytes(f, &som, sizeof(som));

	crypte();
	const bool flushed = fflush(f) == 0;
	const bool closed = fclose(f) == 0;
	if (!valid || !flushed || !closed) {
		std::error_code ec;
		std::filesystem::remove(temporary, ec);
		return false;
	}
	std::string error;
	if (!RuntimePaths::commitTemporaryFile(temporary, destination, error)) {
		std::error_code ec;
		std::filesystem::remove(temporary, ec);
		return false;
	}
	return true;
}

bool HiScores::load(const char * file)
{
	FILE *f = fopen(file, "rb");

	if (f == NULL)
		return false;

	int loaded_scores[HS_NB_SCORES]{};
	char loaded_names[HS_NB_SCORES][HS_NAME_LENGTH]{};
	int som = 0;
	bool valid = true;
	for (int i = 0; i < HS_NB_SCORES; i++) {
		valid = valid && readBytes(f, &loaded_scores[i], sizeof(loaded_scores[i]));
		valid = valid && readBytes(f, loaded_names[i], HS_NAME_LENGTH);
	}
	valid = valid && readBytes(f, &som, sizeof(som)) && fgetc(f) == EOF;
	fclose(f);
	if (!valid) return false;

	for (int i = 0; i < HS_NB_SCORES; ++i) {
		const std::uint32_t mask = 0x35674a1fU << i;
		loaded_scores[i] = static_cast<int>(
		    static_cast<std::uint32_t>(loaded_scores[i]) ^ mask);
		for (int j = 0; j < HS_NAME_LENGTH; j += 4) {
			std::uint32_t word = 0;
			memcpy(&word, &loaded_names[i][j], sizeof(word));
			word ^= mask;
			memcpy(&loaded_names[i][j], &word, sizeof(word));
		}
		if (memchr(loaded_names[i], '\0', HS_NAME_LENGTH) == nullptr) return false;
	}

	std::int64_t checksum = 0;

	for (int i = 0; i < HS_NB_SCORES; i++) {
		checksum += loaded_scores[i];

		for (int j = 0; j < HS_NAME_LENGTH; j++)
			checksum += loaded_names[i][j];
	}
	if (som != static_cast<int>(checksum)) return false;

	for (int i = 0; i < HS_NB_SCORES; ++i) {
		scores[i] = loaded_scores[i];
		memcpy(names[i], loaded_names[i], HS_NAME_LENGTH);
	}
	return true;
}
