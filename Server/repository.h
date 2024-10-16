#pragma once
#include <unordered_map>

#include "database.h"
#include "document.h"

class Repository {
public:
	void process();
private:
	void registerUser();
	void loginUser();

	void createDoc();
	void loadDoc();
	void writeToDoc();
	void eraseFromDoc();

	db::Database<db::User> userDb;
	db::Database<db::Doc> docDb;

	std::unordered_map<std::string, Document> userIdToDocObj;
	std::unordered_map<std::string, std::string> accessCodeToUserId;
};