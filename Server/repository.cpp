#include <filesystem>
#include <sstream>

#include "repository.h"
#pragma push_macro("ERROR")
#undef ERROR

Repository::Repository(const std::string& userDbPath, const std::string& docDbPath, logs::Logger& logger):
	logger(logger),
	userDb(userDbPath, logger),
	docDb(docDbPath, logger) {}


Response Repository::process(msg::Buffer& buffer) {
	if (buffer.size == 1) {
		return newConnection(buffer);
	}

	auto header = msg::Header::parse(buffer);
	switch (header.type) {
	case msg::MessageType::write:
		return writeToDoc(buffer);
	case msg::MessageType::erase:
		return eraseFromDoc(buffer);
	case msg::MessageType::load:
		return loadDoc(buffer);
	case msg::MessageType::create:
		return createDoc(buffer);
	case msg::MessageType::join:
		return joinToDoc(buffer);
	case msg::MessageType::login:
		return loginUser(buffer);
	case msg::MessageType::registration:
		return registerUser(buffer);
	}
	logger.log(logs::Level::ERROR, "Unknown header type in incoming message");
}

Response Repository::newConnection(msg::Buffer& buffer) {
	logger.log(logs::Level::DEBUG, "Thread ", std::this_thread::get_id(), " got new connection");
	return { buffer, ResponseType::none };
}

Response Repository::writeToDoc(msg::Buffer& buffer) {
	auto msg = msg::Write::parse(buffer);
	std::scoped_lock loc{userActiveDocLock};
	auto it = userActiveDoc.find(msg.token);
	if (it == userActiveDoc.end()) {
		return respondError(buffer, msg.header.version, "Write error");
	}
	if (it->second->setCursorPos(msg.cursorPos)) {
		it->second->write(msg.text);
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] wrote '", msg.text, "'");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on write msg!");
	}
	return { buffer, ResponseType::broadcast };
}

Response Repository::eraseFromDoc(msg::Buffer& buffer) {
	auto msg = msg::Erase::parse(buffer);
	std::scoped_lock loc{userActiveDocLock};
	auto it = userActiveDoc.find(msg.token);
	if (it == userActiveDoc.end()) {
		return respondError(buffer, msg.header.version, "Erase error");
	}
	if (it->second->setCursorPos(msg.cursorPos)) {
		it->second->erase(msg.eraseSize);
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] erased '", msg.eraseSize, "'");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on erase msg!");
	}
	return { buffer, ResponseType::broadcast };
}

Response Repository::loadDoc(msg::Buffer& buffer) {
	auto msg = msg::Load::parse(buffer);
	buffer.clear();
	db::Doc readDoc = docDb.readWithAttribute(msg.token, 1);
	if (readDoc.uuid.empty()) {
		return respondError(buffer, msg.header.version, "Load document error");
	}
	auto [docTxt, success] = readDocFile(msg.token + "-" + msg.filename);
	if (!success) {
		return respondError(buffer, msg.header.version, "Cannot open file " + msg.filename);
	}
	std::string accessCode = startTrackingDoc(msg.token, docTxt);
	if (accessCode.empty()) {
		return respondError(buffer, msg.header.version, "Server internal error when producing access code. Try again");
	}
	switchActiveDoc(msg.token, accessCode);
	auto response = msg::ServerResponse<2>(msg::MessageType::load, msg.header.version, 0, { docTxt, accessCode });
	response.serializeTo(buffer);
	return { buffer, ResponseType::unicast };
}

Response Repository::createDoc(msg::Buffer& buffer) {
	auto msg = msg::Create::parse(buffer);
	buffer.clear();
	if (userDb.read(msg.token).uuid != msg.token || msg.token.empty()) {
		return respondError(buffer, msg.header.version, "User not found error");
	}
	db::Doc doc{msg.token, msg.filename};
	if(docDb.create(doc).empty()) {
		return respondError(buffer, msg.header.version, "Create document error");
	}
	if (!initDocFile(msg.token + "-" + msg.filename)) {
		return respondError(buffer, msg.header.version, "Document with specified name already exists!");
	}
	std::string accessCode = startTrackingDoc(msg.token, "");
	if (accessCode.empty()) {
		return respondError(buffer, msg.header.version, "Server internal error when producing access code. Try again");
	}
	switchActiveDoc(msg.token, accessCode);
	auto response = msg::ServerResponse<1>(msg::MessageType::create, msg.header.version, 0, { accessCode });
	response.serializeTo(buffer);
	return { buffer, ResponseType::unicast };
}

Response Repository::joinToDoc(msg::Buffer& buffer) {
	auto msg = msg::Join::parse(buffer);
	buffer.clear();
	if (userDb.read(msg.token).uuid != msg.token || msg.token.empty()) {
		return respondError(buffer, msg.header.version, "User not found error");
	}

	auto [docTxt, success] = joinToTrackedDoc(msg.token, msg.accessCode);
	if (!success || !switchActiveDoc(msg.token, msg.accessCode)) {
		return respondError(buffer, msg.header.version, "Invalid access code!");
	}
	auto response = msg::ServerResponse<1>(msg::MessageType::join, msg.header.version, 0, { docTxt });
	response.serializeTo(buffer);
	return { buffer, ResponseType::unicast };
}

Response Repository::loginUser(msg::Buffer& buffer) {
	auto msg = msg::Login::parse(buffer);
	buffer.clear();
	db::User readUser = userDb.readWithAttribute(msg.username, 1);
	if (readUser.uuid.empty() || readUser.password != msg.password) {
		return respondError(buffer, msg.header.version, "Authorization error");
	}
	auto response = msg::ServerResponse<1>(msg::MessageType::login, msg.header.version, 0, { readUser.uuid });
	response.serializeTo(buffer);
	return { buffer, ResponseType::unicast };
}

Response Repository::registerUser(msg::Buffer& buffer) {
	auto msg = msg::Register::parse(buffer);
	db::User user{msg.username, msg.password};
	buffer.clear();
	if (userDb.create(user).empty()) {
		return respondError(buffer, msg.header.version, "Create user error");
	}
	auto response = msg::ServerResponse<1>(msg::MessageType::registration, msg.header.version, 0, { "User successfully created" });
	response.serializeTo(buffer);
	return { buffer, ResponseType::unicast };
}

Response Repository::respondError(msg::Buffer& buffer, const int version, std::string&& errMsg) {
	buffer.clear();
	auto response = msg::ServerResponse<1>(msg::MessageType::error, 1, 1, { std::move(errMsg) });
	response.serializeTo(buffer);
	return { buffer, ResponseType::unicast };
}

std::pair<std::string, bool> Repository::joinToTrackedDoc(const std::string& userId, const std::string& accessCode) {
	std::scoped_lock lock{docMapLock};
	auto it = accessCodeToDoc.find(accessCode);
	if (it == accessCodeToDoc.end()) {
		return { "", false };
	}
	it->second.userIds.push_back(userId);
	return { it->second.doc->getText(), true };
}

std::string Repository::startTrackingDoc(const std::string& userId, const std::string& txt) {
	std::string accessToken = db::generateAccessCode();
	std::scoped_lock lock{docMapLock, userActiveDocLock};
	auto [it, newOne] = accessCodeToDoc.try_emplace(accessToken, DocData{ txt, userId });
	if (!newOne) {
		return "";
	}
	auto [it2, newOne2] = userActiveDoc.try_emplace(userId, it->second.doc);
	if (!newOne2) {
		userActiveDoc[userId] = it->second.doc;
	}
	return accessToken;
}

bool Repository::initDocFile(const std::string& filename) {
	struct stat buffer;
	if (std::filesystem::exists(filename)) {
		return false;
	}
	std::ofstream file(filename, std::ios::out);
	if (!file) {
		return false;
	}
	return true;
}

std::pair<std::string, bool> Repository::readDocFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::in);
	if (!file) {
		return { "", false };
	}
	std::stringstream ss;
	ss << file.rdbuf();
	return { ss.str(), true };
}

bool Repository::switchActiveDoc(const std::string& userId, const std::string& accessCode) {
	std::scoped_lock lock{docMapLock, userActiveDocLock};
	auto it = accessCodeToDoc.find(accessCode);
	if (it == accessCodeToDoc.end()) {
		return false;
	}
	auto [it2, newOne] = userActiveDoc.try_emplace(userId, it->second.doc);
	if (!newOne) {
		userActiveDoc[userId] = it->second.doc;
	}
	return true;
}

#pragma pop_macro("ERROR")