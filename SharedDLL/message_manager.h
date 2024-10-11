#pragma once
#include <winsock2.h>
#include <windows.h>
#include <memory>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

#ifdef SHAREDDLL_EXPORTS
#define MESSAGE_API __declspec(dllexport)
#else
#define MESSAGE_API __declspec(dllimport)
#endif

class MESSAGE_API MessageManager {
public:
	using OneByteInt = unsigned char;
	using BufferAndSize = std::pair<std::unique_ptr<char[]>, size_t>;

	enum class MessageType {write, erase};
	MessageManager();

	int sendMessage(const SOCKET socket, const BufferAndSize& buffAndSize, const int flags) const;
	int recvMessage(const SOCKET socket, BufferAndSize& buffAndSize, const int maxSize, const int flags) const;

	static int parseNumber(const BufferAndSize& buffAndSize, const int pos, const int count);
	template<typename... Args>
	BufferAndSize buildMessage(const MessageType type, const Args&...) const;
	template<typename... Args>
	std::tuple<Args...> unpackMessage(const BufferAndSize& buffAndSize) const;
private:
	BufferAndSize buildWriteMessage(const OneByteInt command, const OneByteInt version, const OneByteInt isResponse, const COORD& cursorPos, const std::string& msg) const;
	BufferAndSize buildEraseMessage(const OneByteInt command, const OneByteInt version, const OneByteInt isResponse, const COORD& cursorPos, const int eraseSize) const;
};
