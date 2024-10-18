#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <sstream>

#include "logger.h"

#pragma push_macro("ERROR")
#undef ERROR

namespace db {

	std::string generateUUID();
	std::string generateAccessCode();
	bool validateStrField(const std::string& str, const std::string& notAllowedLetters);

	template<int... UNIQUENESS_MASK>
	struct Data {
	public:
		Data(const std::string& uuid, const std::string& name) :
			uuid(uuid),
			name(name) {}
		virtual bool valid() const = 0;
		virtual std::string str() const = 0;
		virtual std::vector<std::string> row() const = 0;
		const std::array<bool, sizeof...(UNIQUENESS_MASK)>& uniqueMask() const {
			return uniqueKeys;
		}
		const std::string uuid;
		const std::string name;
		static constexpr std::array<bool, sizeof...(UNIQUENESS_MASK)> uniqueKeys = { UNIQUENESS_MASK... };
	
	};

	struct User: public Data<1, 1, 0> {
		User() :
			Data("", "USER"),
			username(""),
			password("") {}
		User(const std::string& username, const std::string& password) :
			Data("", "USER"),
			username(username),
			password(password) {}
		User(const std::string& uuid, const User& user) :
			Data(uuid, "USER"),
			username(user.username),
			password(user.password) {}
		User(const std::vector<std::string>& row) :
			Data(row[0], "USER"),
			username(row[1]),
			password(row[2]) {}

		bool valid() const override {
			if (!validateStrField(username, ",\n") || !validateStrField(password, ",\n")) {
				return false;
			}
			return true;
		}
		std::string str() const override {
			return uuid + "," + username + "," + password;
		}
		std::vector<std::string> row() const override {
			return { uuid, username, password };
		}
		std::string username;
		std::string password;
	};

	struct Doc: public Data<1, 0, 0> {
		Doc() :
			Data("", "DOC"),
			userId(),
			filename() {}
		Doc(const std::string& userId, const std::string& filename) :
			Data("", "DOC"),
			userId(userId),
			filename(filename) {}
		Doc(const std::string& uuid, const Doc& doc) :
			Data(uuid, "DOC"),
			userId(doc.userId),
			filename(doc.filename) {}
		Doc(const std::vector<std::string>& row) :
			Data(row[0], "DOC"),
			userId(row[1]),
			filename(row[2]) {}

		bool valid() const override {
			if (!validateStrField(filename, ",\n")) {
				return false;
			}
			return true;
		}
		std::string str() const override {
			return uuid + "," + userId + ',' + filename;
		}
		std::vector<std::string> row() const override {
			return { uuid, userId, filename };
		}
		std::string userId;
		std::string filename;
	};

	template<typename OBJ>
	class Database {
	public:
		Database(const std::string& dbPath, logs::Logger& logger):
			dbPath(dbPath),
			logger(logger) {};

		const std::string create(OBJ& obj) {
			if (!obj.valid()) {
				logger.log(logs::Level::ERROR, obj.name + "is not valid: " + obj.str());
				return "";
			}

			std::ofstream db(dbPath, std::ostream::out | std::ostream::app);
			if (!db) {
				logger.log(logs::Level::ERROR, "Cannot open: " + dbPath);
				return "";
			}

			std::string uuid = obj.uuid.empty() ? generateUUID() : obj.uuid;
			OBJ objToDb{ uuid, obj };
			if (checkUniqueness(objToDb)) {
				db << objToDb.str() << "\n" << std::flush;
				logger.log(logs::Level::INFO, obj.name + "added: " + obj.str());
				return uuid;
			}
			return "";
		}

		OBJ read(const std::string& uuid) {
			auto rowDb = getRowWithUuid(uuid);
			if (rowDb.empty()) {
				return OBJ{};
			}
			return OBJ{ rowDb };
		}

		OBJ readWithAttribute(const std::string& attr, const int pos) {
			auto rowDb = getRowWithAttr(attr, pos);
			if (rowDb.empty()) {
				return OBJ{};
			}
			return OBJ{ rowDb };
		}
		
		bool update(const OBJ& newObj) {
			if (!newObj.valid()) {
				logger.log(logs::Level::ERROR, newObj.name + "is not valid: " + newObj.str());
				return false;
			}				
			if (editRowWithUuid(newObj.uuid, newObj.str() + "\n")) {
				logger.log(logs::Level::INFO, newObj.name + " updated: " + newObj.str());
				return true;
			}
			logger.log(logs::Level::INFO, newObj.name + " not found: " + newObj.str());
			return false;
		}

		bool erase(const std::string& uuid) {
			if (editRowWithUuid(uuid, "")) {
				logger.log(logs::Level::INFO, uuid + " deleted from " + dbPath);
				return true;
			}
			logger.log(logs::Level::INFO, "Obj not found: " + uuid + " in " + dbPath);
			return false;
		}

	private:
		bool editRowWithUuid(const std::string& uuid, const std::string& newRow) const {
			std::ifstream dbIn(dbPath, std::ios::in);
			if (!dbIn) {
				logger.log(logs::Level::ERROR, "Cannot open: " + dbPath);
				return false;
			}

			std::string row;
			std::vector<std::string> lines;
			while (std::getline(dbIn, row)) {
				lines.push_back(row);
			}
			dbIn.close();

			std::string fileContent;
			for (auto& line : lines) {
				std::string rowUuid = line.substr(0, line.find(","));
				if (rowUuid == uuid) {
					fileContent += newRow;
				}
				else {
					fileContent += line + '\n';
				}
			}

			std::ofstream dbOut(dbPath, std::ios::out);
			if (!dbOut) {
				logger.log(logs::Level::ERROR, "Cannot open: " + dbPath);
				return false;
			}
			dbOut << fileContent;
			return true;
		}

		std::vector<std::string> getRowWithUuid(const std::string& uuid) {
			std::ifstream db(dbPath, std::istream::in);
			if (!db) {
				logger.log(logs::Level::ERROR, "Cannot open: " + dbPath + " for read");
				return {};
			}

			std::string rowStr;
			while (std::getline(db, rowStr)) {
				std::string rowUuid = rowStr.substr(0, rowStr.find(","));
				if (rowUuid == uuid) {
					return parseRow(rowStr, ',');
				}
			}
			logger.log(logs::Level::DEBUG, "Not found obj with uuid: " + uuid + " from db", dbPath);
			return {};
		}

		std::vector<std::string> getRowWithAttr(const std::string& attr, const int pos) {
			std::ifstream db(dbPath, std::istream::in);
			if (!db) {
				logger.log(logs::Level::ERROR, "Cannot open: " + dbPath + " for read");
				return {};
			}

			std::string rowStr;
			while (std::getline(db, rowStr)) {
				auto parsedRow = parseRow(rowStr, ',');
				if (parsedRow[pos] == attr) {
					return parsedRow;
				}
			}
			logger.log(logs::Level::DEBUG, "Not found obj with attr: " + attr + " from db", dbPath);
			return {};
		}

		bool checkUniqueness(const OBJ& obj) const {
			std::ifstream db(dbPath, std::istream::in);
			if (!db) {
				logger.log(logs::Level::ERROR, "Cannot open: " + dbPath + " for read");
				return "";
			}

			auto rowObj = obj.row();
			auto uniqueMask = obj.uniqueMask();
			std::string rowStr;
			while (std::getline(db, rowStr)) {
				auto rowDb = parseRow(rowStr, ',');
				for (int i = 0; i < rowObj.size(); i++) {
					if (!uniqueMask[i]) {
						continue;
					}
					if (rowDb[i] == rowObj[i]) {
						logger.log(logs::Level::ERROR, "Such object already exists in db " + dbPath + ": " + obj.str());
						return false;
					}
				}
			}
			return true;
		}

		std::vector<std::string> parseRow(std::string& row, char delimiter) const {
			std::vector<std::string> parsedRow;
			int offset = 0;
			int delimiterPos = 0;
			while ((delimiterPos = row.find(delimiter, offset)) != std::string::npos) {
				parsedRow.emplace_back(row.substr(offset, delimiterPos - offset));
				offset = delimiterPos + 1;
			}
			parsedRow.emplace_back(row.substr(offset, row.size() - offset));
			return parsedRow;
		}

		std::string dbPath;
		logs::Logger& logger;
	};

}

#pragma pop_macro("ERROR")