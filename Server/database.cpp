#include <random>

#include "database.h"

namespace db {
	auto getRandomEngine() {
		std::random_device device;
		std::random_device::result_type data[(std::mt19937::state_size - 1) / sizeof(device()) + 1];
		std::generate(std::begin(data), std::end(data), std::ref(device));
		std::seed_seq seed{std::begin(data), std::end(data)};
		return std::mt19937(seed);
	}

	static auto randomEngine = getRandomEngine();

	std::string generateUUID() {
		std::uniform_int_distribution<> dist1(0, 15);
		std::uniform_int_distribution<> dist2(8, 11);

		std::stringstream ss;
		ss << std::hex;
		for (int i = 0; i < 8; i++) {
			ss << dist1(randomEngine);
		}
		ss << "-";
		for (int i = 0; i < 4; i++) {
			ss << dist1(randomEngine);
		}
		ss << "-4";
		for (int i = 0; i < 3; i++) {
			ss << dist1(randomEngine);
		}
		ss << "-";
		ss << dist2(randomEngine);
		for (int i = 0; i < 3; i++) {
			ss << dist1(randomEngine);
		}
		ss << "-";
		for (int i = 0; i < 12; i++) {
			ss << dist1(randomEngine);
		}
		return ss.str();
	}

	std::string generateAccessCode() {
		std::uniform_int_distribution<> dist1(0, 15);

		std::stringstream ss;
		ss << std::hex;
		for (int i = 0; i < 6; i++) {
			ss << dist1(randomEngine);
		}
		return ss.str();
	}

	bool validateStrField(const std::string& str, const std::string& notAllowedLetters) {
		for (const auto letter : notAllowedLetters) {
			if (str.find(letter) != std::string::npos) {
				return false;
			}
		}
		return true;
	}
}
