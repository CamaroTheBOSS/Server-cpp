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

	template<typename T>
	int parse(T& obj, Buffer& buffer, const int offset) {
		memcpy(&obj, buffer.get() + offset, sizeof(T));
		return sizeof(T);
	}
	int parse(std::string& obj, Buffer& buffer, const int offset) {
		obj = std::string{ buffer.get() + offset };
		return obj.size() + 1;
	}

	Header::Header(MessageType type, const int version, const int errCode) :
		type(type),
		version(version),
		errCode(errCode) {}

	Header Header::parse(Buffer& buffer) {
		OneByteInt versionBuf, typeBuf, errCodeBuf;
		memcpy(&versionBuf, buffer.get(), sizeof(OneByteInt));
		memcpy(&typeBuf, buffer.get() + sizeof(OneByteInt), sizeof(OneByteInt));
		memcpy(&errCodeBuf, buffer.get() + 2 * sizeof(OneByteInt), sizeof(OneByteInt));
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
		password(password) {}

	Register Register::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		const std::string username{ buffer.get() + header.size};
		const std::string password{ buffer.get() + header.size + username.size() + 1};
		return Register{ header.version, header.errCode, username, password};
	}

	void Register::serializeTo(Buffer& buffer) const {
		header.serializeTo(buffer);
		buffer.add(&username);
		buffer.add(&password);
	}


	Login::Login(const int version, const int errCode, const std::string& username, const std::string& password) :
		header(MessageType::login, version, errCode),
		username(username),
		password(password) {}

	Login Login::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		const std::string username{ buffer.get() + header.size};
		const std::string password{ buffer.get() + header.size + username.size() + 1};
		return Login{ header.version, header.errCode, username, password };
	}

	void Login::serializeTo(Buffer& buffer) const {
		header.serializeTo(buffer);
		buffer.add(&username);
		buffer.add(&password);
	}


	Create::Create(const int version, const int errCode, const std::string& token, const std::string& filename) :
		header(MessageType::create, version, errCode),
		token(token),
		filename(filename) {}

	Create Create::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		const std::string token{ buffer.get() + header.size};
		const std::string filename{ buffer.get() + header.size + token.size() + 1};
		return Create{ header.version, header.errCode, token, filename };
	}

	void Create::serializeTo(Buffer& buffer) const {
		header.serializeTo(buffer);
		buffer.add(&token);
		buffer.add(&filename);
	}


	Load::Load(const int version, const int errCode, const std::string& token, const std::string& filename) :
		header(MessageType::load, version, errCode),
		token(token),
		filename(filename) {}

	Load Load::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		const std::string token{ buffer.get() + header.size};
		const std::string filename{ buffer.get() + header.size + token.size() + 1};
		return Load{ header.version, header.errCode, token, filename };
	}

	void Load::serializeTo(Buffer& buffer) const {
		header.serializeTo(buffer);
		buffer.add(&token);
		buffer.add(&filename);
	}


	Join::Join(const int version, const int errCode, const std::string& token, const std::string& accessCode) :
		header(MessageType::join, version, errCode),
		token(token),
		accessCode(accessCode) {}

	Join Join::parse(Buffer& buffer) {
		Header header = Header::parse(buffer);
		const std::string token{ buffer.get() + header.size};
		const std::string accessCode{ buffer.get() + header.size + token.size() + 1};
		return Join{ header.version, header.errCode, token, accessCode };
	}

	void Join::serializeTo(Buffer& buffer) const {
		header.serializeTo(buffer);
		buffer.add(&token);
		buffer.add(&accessCode);
	}


	Write::Write(const int version, const int errCode, const std::string& token, const COORD& cursorPos, const std::string& text) :
		header(MessageType::write, version, errCode),
		token(token),
		cursorPos(cursorPos),
		text(text),
		size(header.size + 2 * sizeof(u_short) + text.size() + 1) {}

	void Write::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		u_short cursorX = htons(cursorPos.X);
		u_short cursorY = htons(cursorPos.Y);
		buffer.add(&token);
		buffer.add(&cursorX);
		buffer.add(&cursorY);
		buffer.add(&text);
	}

	Write Write::parse(Buffer& buffer, const int pos) {
		Header header = Header::parse(buffer);
		const std::string token{ buffer.get() + header.size};
		u_short cursorX, cursorY;
		memcpy(&cursorX, buffer.get() + header.size + token.size() + 1, sizeof(u_short));
		memcpy(&cursorY, buffer.get() + header.size + token.size() + 1 + sizeof(u_short), sizeof(u_short));
		const std::string text{ buffer.get() + header.size + token.size() + 1 + 2 * sizeof(u_short) };
		SHORT cursorPosX = static_cast<SHORT>(ntohs(cursorX));
		SHORT cursorPosY = static_cast<SHORT>(ntohs(cursorY));
		return Write{ header.version, header.errCode, token, COORD{cursorPosX, cursorPosY}, text };
	}

	Erase::Erase(const int version, const int errCode, const std::string& token, const COORD& cursorPos, const int eraseSize) :
		header(MessageType::erase, version, errCode),
		token(token),
		cursorPos(cursorPos),
		eraseSize(eraseSize),
		size(header.size + 2 * sizeof(u_short) + sizeof(u_long)) {}

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

	Erase Erase::parse(Buffer& buffer, const int pos) {
		Header header = Header::parse(buffer);
		const std::string token{ buffer.get() + header.size};
		u_short cursorX, cursorY; u_long eraseSizeBuf;
		memcpy(&cursorX, buffer.get() + header.size + token.size() + 1, sizeof(u_short));
		memcpy(&cursorY, buffer.get() + header.size + token.size() + 1 + sizeof(u_short), sizeof(u_short));
		memcpy(&eraseSizeBuf, buffer.get() + token.size() + 1 + header.size + 2 * sizeof(u_short), sizeof(u_long));
		SHORT cursorPosX = static_cast<SHORT>(ntohs(cursorX));
		SHORT cursorPosY = static_cast<SHORT>(ntohs(cursorY));
		int eraseSize = static_cast<int>(ntohl(eraseSizeBuf));
		return Erase{ header.version, header.errCode, token, COORD{cursorPosX, cursorPosY}, eraseSize };
	}

}