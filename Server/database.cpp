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

	bool validateStrField(const std::string& str, const std::string& notAllowedLetters) {
		for (const auto letter : notAllowedLetters) {
			if (str.find(letter) != std::string::npos) {
				return false;
			}
		}
		return true;
	}
}

//bool Database::updateUser(const UserData& userData) const {
//	if (!userData.valid()) {
//		logger.log(logs::Level::ERROR, "UserData is not valid user: " + userData.toString());
//		return "";
//	}
//
//	if (editRowWithUuid(userDb, userData.uuid, userData.toString() + "\n")) {
//		logger.log(logs::Level::INFO, "Updated user: " + userData.toString());
//		return true;
//	}
//	logger.log(logs::Level::INFO, "User not found: " + userData.toString());
//	return false;
//}
//
//bool Database::updateDoc(const DocData& docData) const {
//	if (!docData.valid()) {
//		logger.log(logs::Level::ERROR, "DocData is not valid doc: " + docData.toString());
//		return "";
//	}
//
//	if (editRowWithUuid(docDb, docData.uuid, docData.toString() + "\n")) {
//		logger.log(logs::Level::INFO, "Updated doc: " + docData.toString());
//		return true;
//	}
//	logger.log(logs::Level::INFO, "Doc not found: " + docData.toString());
//	return false;
//}
//
//bool Database::deleteUser(const std::string& userId) const {
//	if (editRowWithUuid(userDb, userId, "")) {
//		logger.log(logs::Level::INFO, "Deleted user: " + userId);
//		return true;
//	}
//	logger.log(logs::Level::INFO, "User not found: " + userId);
//	return false;
//}
//
//bool Database::deleteDoc(const std::string& docId) const {
//	if (editRowWithUuid(docDb, docId, "")) {
//		logger.log(logs::Level::INFO, "Updated doc: " + docId);
//		return true;
//	}
//	logger.log(logs::Level::INFO, "Doc not found: " + docId);
//	return false;
//}
//
//bool Database::editRowWithUuid(const std::string& dbFile, const std::string& uuid, const std::string& newLine) const {
//	std::fstream db(dbFile, std::fstream::in | std::fstream::out | std::fstream::app);
//	if (!db) {
//		logger.log(logs::Level::ERROR, "Cannot open: " + docDb);
//		return false;
//	}
//
//	std::string row;
//	while (std::getline(db, row)) {
//		std::string rowUuid = row.substr(0, row.find(","));
//		if (rowUuid == uuid) {
//			db << newLine << std::flush;
//			db.close();
//			return true;
//		}
//	}
//	db.close();
//	return false;
//}
