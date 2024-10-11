#include "pch.h"
#include "message_manager.h";
#include <WS2tcpip.h>
#include <assert.h>

MessageManager::MessageManager() {}

template<typename... Args>
MessageManager::BufferAndSize MessageManager::buildMessage(const MessageType type, const Args&... args) const {
	if (type == MessageType::write) {
		return buildWriteMessage(type, args);
	}
	else if (type == MessageType::erase) {
		return buildEraseMessage(type, args);
	}
	return { std::make_unique<char[]>(0), 0 };
}

template<typename... Args>
std::tuple<Args...> MessageManager::unpackMessage(const BufferAndSize& buffAndSize) const {
	const OneByteInt version = MessageManager::parseNumber(buffAndSize, 0, sizeof(OneByteInt));
	const MessageType command = static_cast<MessageType>(MessageManager::parseNumber(buffAndSize, sizeof(OneByteInt), sizeof(OneByteInt)));
	const OneByteInt isResponse = MessageManager::parseNumber(buffAndSize, 2 * sizeof(OneByteInt), sizeof(OneByteInt));
	const short cursorX = MessageManager::parseNumber(buffAndSize, 3 * sizeof(OneByteInt), sizeof(SHORT));
	const short cursorY = MessageManager::parseNumber(buffAndSize, 3 * sizeof(OneByteInt) + sizeof(SHORT), sizeof(SHORT));
	if (command == MessageType::write) {
		std::string msg{buffAndSize.first.get() + 3 * sizeof(OneByteInt) + 2 * sizeof(SHORT)};
		return std::tuple<OneByteInt, MessageType, OneByteInt, COORD, std::string>(version, command, isResponse, COORD{cursorX, cursorY}, msg);
	}
	else if (command == MessageType::erase) {
		const int eraseSize = MessageManager::parseNumber(buffAndSize, 3 * sizeof(OneByteInt) + 2 * sizeof(SHORT), sizeof(int));
		return std::tuple<OneByteInt, MessageType, OneByteInt, COORD, int>(version, command, isResponse, COORD{ cursorX, cursorY }, eraseSize);
	}
	
	return std::tuple<int>(0);
}


int MessageManager::parseNumber(const BufferAndSize& buffAndSize, const int pos, const int count) {
	assert(buffAndSize.second >= pos + count && "Exceeded buffer size");
	int val = 0;
	for (int i = 0; i < count; i++) {
		val |= buffAndSize.first.get()[pos + count] << 8 * (count - i - 1);
	}
	return val;
}

MessageManager::BufferAndSize MessageManager::buildWriteMessage(const OneByteInt command, const OneByteInt version, const OneByteInt isResponse, const COORD& cursorPos, const std::string& msg) const {
	size_t msgSize = 3 * sizeof(OneByteInt) + 2 * sizeof(SHORT) + msg.size() + 1;
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(msgSize);
	memcpy(buffer.get(), &version, sizeof(OneByteInt));
	memcpy(buffer.get() + sizeof(OneByteInt), &command, sizeof(OneByteInt));
	memcpy(buffer.get() + 2 * sizeof(OneByteInt), &isResponse, sizeof(OneByteInt));
	memcpy(buffer.get() + 3 * sizeof(OneByteInt), &cursorPos.X, sizeof(SHORT));
	memcpy(buffer.get() + 3 * sizeof(OneByteInt) + sizeof(SHORT), &cursorPos.Y, sizeof(SHORT));
	memcpy(buffer.get() + 3 * sizeof(OneByteInt) + 2 * sizeof(SHORT), msg.c_str(), msg.size() + 1);
	return { std::move(buffer), msgSize };
}

MessageManager::BufferAndSize MessageManager::buildEraseMessage(const OneByteInt command, const OneByteInt version, const OneByteInt isResponse, const COORD& cursorPos, const int eraseSize) const {
	size_t msgSize = 3 * sizeof(OneByteInt) + 2 * sizeof(SHORT) + sizeof(eraseSize);
	std::unique_ptr<char[]> buffer = std::make_unique<char[]>(msgSize);
	memcpy(buffer.get(), &version, sizeof(OneByteInt));
	memcpy(buffer.get() + sizeof(OneByteInt), &command, sizeof(OneByteInt));
	memcpy(buffer.get() + 2 * sizeof(OneByteInt), &isResponse, sizeof(OneByteInt));
	memcpy(buffer.get() + 3 * sizeof(OneByteInt), &cursorPos.X, sizeof(SHORT));
	memcpy(buffer.get() + 3 * sizeof(OneByteInt) + sizeof(SHORT), &cursorPos.Y, sizeof(SHORT));
	memcpy(buffer.get() + 3 * sizeof(OneByteInt) + 2 * sizeof(SHORT), &eraseSize, sizeof(int));
	return { std::move(buffer), msgSize };
}

int MessageManager::sendMessage(const SOCKET socket, const BufferAndSize& buffAndSize, const int flags) const {
	int sendBytes = send(socket, buffAndSize.first.get(), buffAndSize.second, flags);
	return sendBytes;
}

int MessageManager::recvMessage(const SOCKET socket, BufferAndSize& buffAndSize, const int maxSize, const int flags) const {
	int recvBytes = recv(socket, buffAndSize.first.get(), maxSize, flags);
	buffAndSize.second = recvBytes;
	return recvBytes;
}
