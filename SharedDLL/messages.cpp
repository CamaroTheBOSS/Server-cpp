#include "pch.h"
#include "messages.h"

namespace msg {

	Buffer::Buffer(const int capacity) :
		data(std::make_unique<char[]>(capacity)),
		size(0),
		capacity(capacity) {}

	char* Buffer::get() {
		return data.get();
	}

	void Buffer::clear() {
		memset(data.get(), 0, capacity);
		size = 0;
	}

	template<typename T>
	int parseObj(T& obj, Buffer& buffer, const int offset) {
		memcpy(&obj, buffer.get() + offset, sizeof(T));
		return sizeof(T);
	}
	int parseObj(std::string& obj, Buffer& buffer, const int offset) {
		obj = std::string{ buffer.get() + offset };
		return obj.size() + 1;
	}

	template<typename... Args>
	int parseMultipleObjs(Buffer& buffer, int pos, Args&... args) {
		([&] {
			pos += parseObj(args, buffer, pos);
			} (), ...);
		return pos;
	}

	Header::Header(MessageType type, const int version, const int errCode) :
		type(type),
		version(version),
		errCode(errCode) {}

	Header Header::parse(Buffer& buffer) {
		OneByteInt versionBuf, typeBuf, errCodeBuf; int pos = 0;
		parseMultipleObjs(buffer, 0, versionBuf, typeBuf, errCodeBuf);
		const MessageType type = static_cast<MessageType>(typeBuf);
		return Header{ type, versionBuf, errCodeBuf };
	}

	void Header::serializeTo(Buffer& buffer) const {
		OneByteInt versionByte = static_cast<OneByteInt>(version);
		OneByteInt typeByte = static_cast<OneByteInt>(type);
		OneByteInt errCodeByte = static_cast<OneByteInt>(errCode);
		buffer.add(&versionByte);
		buffer.add(&typeByte);
		buffer.add(&errCodeByte);
	}


	Register::Register(const int version, const int errCode, const std::string& username, const std::string& password) :
		header(MessageType::registration, version, errCode),
		username(username),
		password(password),
		size(header.size + username.size() + password.size() + 2) {}

	Register Register::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		std::string username, password;
		parseMultipleObjs(buffer, header.size, username, password);
		return Register{ header.version, header.errCode, username, password};
	}

	void Register::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		buffer.add(&username);
		buffer.add(&password);
	}


	Login::Login(const int version, const int errCode, const std::string& username, const std::string& password) :
		header(MessageType::login, version, errCode),
		username(username),
		password(password),
		size(header.size + username.size() + password.size() + 2) {}

	Login Login::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		std::string username, password;
		parseMultipleObjs(buffer, header.size, username, password);
		return Login{ header.version, header.errCode, username, password };
	}

	void Login::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		buffer.add(&username);
		buffer.add(&password);
	}


	Create::Create(const int version, const int errCode, const std::string& token, const std::string& filename) :
		header(MessageType::create, version, errCode),
		token(token),
		filename(filename),
		size(header.size + token.size() + filename.size() + 2) {}

	Create Create::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		std::string token, filename;
		parseMultipleObjs(buffer, header.size, token, filename);
		return Create{ header.version, header.errCode, token, filename };
	}

	void Create::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		buffer.add(&token);
		buffer.add(&filename);
	}


	Load::Load(const int version, const int errCode, const std::string& token, const std::string& filename) :
		header(MessageType::load, version, errCode),
		token(token),
		filename(filename),
		size(header.size + token.size() + filename.size() + 2) {}

	Load Load::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		std::string token, filename;
		parseMultipleObjs(buffer, header.size, token, filename);
		return Load{ header.version, header.errCode, token, filename };
	}

	void Load::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		buffer.add(&token);
		buffer.add(&filename);
	}


	Join::Join(const int version, const int errCode, const std::string& token, const std::string& accessCode) :
		header(MessageType::join, version, errCode),
		token(token),
		accessCode(accessCode),
		size(header.size + token.size() + accessCode.size() + 2) {}

	Join Join::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		std::string token, accessCode;
		parseMultipleObjs(buffer, header.size, token, accessCode);
		return Join{ header.version, header.errCode, token, accessCode };
	}

	void Join::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		buffer.add(&token);
		buffer.add(&accessCode);
	}


	Write::Write(const int version, const int errCode, const std::string& token, const COORD& cursorPos, const std::string& text) :
		header(MessageType::write, version, errCode),
		token(token),
		cursorPos(cursorPos),
		text(text),
		size(header.size + 2 * sizeof(u_short) + token.size() + text.size() + 2) {}

	void Write::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		u_short cursorX = htons(cursorPos.X);
		u_short cursorY = htons(cursorPos.Y);
		buffer.add(&token);
		buffer.add(&cursorX);
		buffer.add(&cursorY);
		buffer.add(&text);
	}

	Write Write::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		std::string token, text; u_short cursorX, cursorY;
		parseMultipleObjs(buffer, header.size, token, cursorX, cursorY, text);
		SHORT cursorPosX = static_cast<SHORT>(ntohs(cursorX));
		SHORT cursorPosY = static_cast<SHORT>(ntohs(cursorY));
		return Write{ header.version, header.errCode, token, COORD{cursorPosX, cursorPosY}, text };
	}


	Erase::Erase(const int version, const int errCode, const std::string& token, const COORD& cursorPos, const int eraseSize) :
		header(MessageType::erase, version, errCode),
		token(token),
		cursorPos(cursorPos),
		eraseSize(eraseSize),
		size(header.size + 2 * sizeof(u_short) + sizeof(u_long) + token.size() + 1) {}

	void Erase::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		u_short cursorX = htons(cursorPos.X);
		u_short cursorY = htons(cursorPos.Y);
		u_long eraseSizeByte = htonl(eraseSize);
		buffer.add(&token);
		buffer.add(&cursorX);
		buffer.add(&cursorY);
		buffer.add(&eraseSizeByte);
	}

	Erase Erase::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		std::string token; u_short cursorX, cursorY; u_long eraseSizeBuf;
		parseMultipleObjs(buffer, header.size, token, cursorX, cursorY, eraseSizeBuf);
		SHORT cursorPosX = static_cast<SHORT>(ntohs(cursorX));
		SHORT cursorPosY = static_cast<SHORT>(ntohs(cursorY));
		int eraseSize = static_cast<int>(ntohl(eraseSizeBuf));
		return Erase{ header.version, header.errCode, token, COORD{cursorPosX, cursorPosY}, eraseSize };
	}
}