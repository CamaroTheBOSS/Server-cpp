#pragma once
#include <unordered_map>
#include <mutex>

#include "database.h"
#include "messages.h"
#include "document.h"

struct DocData {
	DocData(std::string txt, const std::string& userId) :
		doc(std::make_shared<Document>(std::move(txt))),
		userIds{ userId } {}
	std::shared_ptr<Document> doc;
	std::vector<std::string> userIds;
};

enum class ResponseType { none, unicast, broadcast };
using Response = std::pair<msg::Buffer&, ResponseType>;

class Repository {
public:
	Repository(const std::string& userDbPath, const std::string& docDbPath, logs::Logger& logger);
	Response process(msg::Buffer& buffer);
private:
	Response registerUser(msg::Buffer& buffer);
	Response loginUser(msg::Buffer& buffer);

	Response createDoc(msg::Buffer& buffer);
	bool initDocFile(const std::string& filename);

	Response loadDoc(msg::Buffer& buffer);
	Response joinToDoc(msg::Buffer& buffer);
	std::pair<std::string, bool> readDocFile(const std::string& filename);

	Response writeToDoc(msg::Buffer& buffer);
	Response eraseFromDoc(msg::Buffer& buffer);
	Response newConnection(msg::Buffer& buffer);

	Response respondError(msg::Buffer& buffer, const int version, std::string&& errMsg);
	
	std::pair<std::string, bool> joinToTrackedDoc(const std::string& userId, const std::string& accessCode);
	std::string startTrackingDoc(const std::string& userId, const std::string& txt);
	bool switchActiveDoc(const std::string& userId, const std::string& accessCode);


	logs::Logger& logger;
	db::Database<db::User> userDb;
	db::Database<db::Doc> docDb;
	std::unordered_map<std::string, std::shared_ptr<Document>> userActiveDoc;
	std::unordered_map<std::string, DocData> accessCodeToDoc;
	std::mutex docMapLock, userActiveDocLock;
};