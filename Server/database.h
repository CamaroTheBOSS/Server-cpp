#pragma once
#include <vector>
#include <string>
#include <fstream>

#include "logger.h"

namespace db {

	namespace random {
		std::string generateUUID();
	}

	bool validateStrField(const std::string& str, const std::string& notAllowedLetters);


struct UserData {
	UserData(const std::string& username, const std::string& password) :
		username(username),
		password(password) {}
	UserData(const std::string uuid, std::string& username, const std::string& password) :
		uuid(uuid),
		username(username),
		password(password) {}

	bool valid() const {
		if (!validateStrField(username, ",\n") || !validateStrField(password, ",\n")) {
			return false;
		}
		return true;
	}
	std::string toString() const {
		return uuid + "," + username + "," + password;
	}
	const std::string uuid = "";
	std::string username;
	std::string password;
};

struct DocData {
	DocData(const std::string& filename) :
		filename(filename) {}
	DocData(const std::string uuid, const std::string& filename) :
		uuid(uuid),
		filename(filename) {}

	bool valid() const {
		if (!validateStrField(filename, ",\n")) {
			return false;
		}
		return true;
	}
	std::string toString() const {
		return uuid + "," + filename;
	}
	const std::string uuid = "";
	std::string filename;
};

class Database {
public:
	Database(std::string& root, logs::Logger& logger);
	std::string createUser(UserData& userData) const;
	std::string createDoc(DocData& docData) const;

	UserData readUser(const std::string& userId) const;
	DocData readDoc(const std::string& docId) const;

	bool updateUser(const UserData& userData) const;
	bool updateDoc(const DocData& docData) const;

	bool deleteUser(const std::string& userId) const;
	bool deleteDoc(const std::string& docId) const;

private:
	bool editRowWithUuid(const std::string& dbFile, const std::string& uuid, const std::string& newLine) const;
	std::vector<std::string> findRowWithUuid(const std::string& dbFile, const std::string& uuid) const;
	std::vector<std::string> parseRow(std::string& row, char delimiter) const;

	std::string root;
	std::string userDb;
	std::string docDb;

	logs::Logger& logger;
};


}
