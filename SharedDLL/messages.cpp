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

	Header::Header(MessageType type, const int version, const int userId) :
		type(type),
		version(version),
		userId(userId) {}

	void Header::serializeTo(Buffer& buffer) {
		OneByteInt versionByte = static_cast<OneByteInt>(version);
		OneByteInt typeByte = static_cast<OneByteInt>(type);
		u_long userIdByte = htonl(userId);
		buffer.add(&versionByte);
		buffer.add(&typeByte);
		buffer.add(&userIdByte);
	}

	Header Header::parse(Buffer& buffer, const int pos) {
		OneByteInt version; OneByteInt typeBuf; u_long userIdBuf;
		memcpy(&version, buffer.get(), sizeof(OneByteInt));
		memcpy(&typeBuf, buffer.get() + sizeof(OneByteInt), sizeof(OneByteInt));
		memcpy(&userIdBuf, buffer.get() + 2 * sizeof(OneByteInt), sizeof(u_long));
		const int userId = static_cast<int>(ntohl(userIdBuf));
		const MessageType type = static_cast<MessageType>(typeBuf);
		return Header{ type, version, userId };
	}

	Write::Write(const int version, const int userId, COORD cursorPos, std::string msg) :
		header(MessageType::write, version, userId),
		cursorPos(std::move(cursorPos)),
		msg(std::move(msg)),
		size(header.size + 2 * sizeof(u_short) + this->msg.size() + 1) {}

	void Write::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		u_short cursorX = htons(cursorPos.X);
		u_short cursorY = htons(cursorPos.Y);
		buffer.add(&cursorX);
		buffer.add(&cursorY);
		buffer.add(&msg);
	}

	Write Write::parse(Buffer& buffer, const int pos) {
		Header header = Header::parse(buffer, 0);
		u_short cursorX, cursorY;
		memcpy(&cursorX, buffer.get() + header.size, sizeof(u_short));
		memcpy(&cursorY, buffer.get() + header.size + sizeof(u_short), sizeof(u_short));
		const std::string msg{ buffer.get() + header.size + 2 * sizeof(u_short) };
		SHORT cursorPosX = static_cast<SHORT>(ntohs(cursorX));
		SHORT cursorPosY = static_cast<SHORT>(ntohs(cursorY));
		return Write{ header.version, header.userId, COORD{cursorPosX, cursorPosY}, msg };
	}

	Sync::Sync(const int version, const int userId, std::string msg) :
		header(MessageType::sync, version, userId),
		msg(std::move(msg)),
		size(header.size + this->msg.size() + 1) {}

	void Sync::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		buffer.add(&msg);
	}

	Sync Sync::parse(Buffer& buffer, const int pos) {
		Header header = Header::parse(buffer, 0);
		const std::string msg{ buffer.get() + header.size };
		return Sync{ header.version, header.userId, msg };
	}

	Erase::Erase(const int version, const int userId, COORD cursorPos, const int eraseSize) :
		header(MessageType::erase, version, userId),
		cursorPos(std::move(cursorPos)),
		eraseSize(eraseSize),
		size(header.size + 2 * sizeof(u_short) + sizeof(u_long)) {}

	void Erase::serializeTo(Buffer& buffer) {
		header.serializeTo(buffer);
		u_short cursorX = htons(cursorPos.X);
		u_short cursorY = htons(cursorPos.Y);
		u_long eraseSizeByte = htonl(eraseSize);
		buffer.add(&cursorX);
		buffer.add(&cursorY);
		buffer.add(&eraseSizeByte);
	}

	Erase Erase::parse(Buffer& buffer, const int pos) {
		Header header = Header::parse(buffer, 0);
		u_short cursorX, cursorY; u_long eraseSizeBuf;
		memcpy(&cursorX, buffer.get() + header.size, sizeof(u_short));
		memcpy(&cursorY, buffer.get() + header.size + sizeof(u_short), sizeof(u_short));
		memcpy(&eraseSizeBuf, buffer.get() + header.size + 2 * sizeof(u_short), sizeof(u_long));
		SHORT cursorPosX = static_cast<SHORT>(ntohs(cursorX));
		SHORT cursorPosY = static_cast<SHORT>(ntohs(cursorY));
		int eraseSize = static_cast<int>(ntohl(eraseSizeBuf));
		return Erase{ header.version, header.userId, COORD{cursorPosX, cursorPosY}, eraseSize };
	}

}