#pragma once
#include <string>
#include <assert.h>
#include <memory>

#include <winsock2.h>
#include "windows.h"

#pragma comment(lib, "Ws2_32.lib")


#ifdef SHAREDDLL_EXPORTS
#define MESSAGE_API __declspec(dllexport)
#else
#define MESSAGE_API __declspec(dllimport)
#endif

namespace msg {

using OneByteInt = unsigned char;

enum class MessageType { write, erase, sync };

class MESSAGE_API Buffer {
public:
	Buffer(const int capacity);

	template<typename T>
	void add(T* val) {
		assert(capacity >= size + sizeof(T) && "Error, buffer size excedeed!");
		memcpy(data.get() + size, val, sizeof(T));
		size += sizeof(T);
	}
	void add(std::string* str) {
		assert(capacity >= size + str->size() + 1 && "Error, buffer size excedeed!");
		memcpy(data.get() + size, str->c_str(), str->size() + 1);
		size += str->size() + 1;
	}
	char* get();

	std::unique_ptr<char[]> data;
	int size;
	const int capacity;
};


class MESSAGE_API Header {
public:
	Header(MessageType type, const int version, const int userId);
	static Header parse(Buffer& buffer, const int pos);
	void serializeTo(Buffer& buffer);

	MessageType type;
	int version;
	int userId;
	const int size = 2 * sizeof(OneByteInt) + sizeof(u_long);
};

class MESSAGE_API Write {
public:
	Write(const int version, const int userId, COORD cursorPos, std::string msg);
	static Write parse(Buffer& buffer, const int pos);
	void serializeTo(Buffer& buffer);

	Header header;
	COORD cursorPos;
	std::string msg;
	int size;
};

class MESSAGE_API Sync {
public:
	Sync(const int version, const int userId, std::string msg);
	static Sync parse(Buffer& buffer, const int pos);
	void serializeTo(Buffer& buffer);

	Header header;
	std::string msg;
	int size;
};

class MESSAGE_API Erase {
public:
	Erase(const int version, const int userId, COORD cursorPos, const int eraseSize);
	static Erase parse(Buffer& buffer, const int pos);
	void serializeTo(Buffer& buffer);

	Header header;
	COORD cursorPos;
	int eraseSize;
	int size;
};

}
