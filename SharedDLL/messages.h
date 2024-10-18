#pragma once
#include <string>
#include <assert.h>
#include <memory>
#include <array>

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
	/*
		Write: Header CursorPos letters (for writing to doc) -> returns same Write msg
		Erase: Header CursorPos eraseSize (for erasing from doc) -> returns same Erase msg
		Login: Header nickname (for login into system -> returning userId) -> returns same login msg
		Create: Header filename (for creating new doc) -> returns Header docId
		Load: Header filename (for loading existing doc) -> returns Header docId
		Join: Header docId (for joining to specific session) -> returns Header documentData
	*/
	enum class MessageType { registration, login, create, load, join, write, erase, error };

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
		void clear();
		char* get();

		std::unique_ptr<char[]> data;
		int size;
		const int capacity;
	};

	class MESSAGE_API Header {
	public:
		Header(MessageType type, const int version, const int errCode);
		static Header parse(Buffer& buffer);
		void serializeTo(Buffer& buffer) const;
		MessageType type;
		int version;
		int errCode;
		const int size = 3 * sizeof(OneByteInt);
	};

	class MESSAGE_API Register {
	public:
		Register(const int version, const int errCode, const std::string& username, const std::string& password);
		static Register parse(Buffer& buffer);
		void serializeTo(Buffer& buffer);

		Header header;
		std::string username;
		std::string password;
		int size;
	};

	class MESSAGE_API Login {
	public:
		Login(const int version, const int errCode, const std::string& username, const std::string& password);
		static Login parse(Buffer& buffer);
		void serializeTo(Buffer& buffer);

		Header header;
		std::string username;
		std::string password;
		int size;
	};

	class MESSAGE_API Create {
	public:
		Create(const int version, const int errCode, const std::string& token, const std::string& filename);
		static Create parse(Buffer& buffer);
		void serializeTo(Buffer& buffer);

		Header header;
		std::string token;
		std::string filename;
		int size;
	};

	class MESSAGE_API Load {
	public:
		Load(const int version, const int errCode, const std::string& token, const std::string& filename);
		static Load parse(Buffer& buffer);
		void serializeTo(Buffer& buffer);

		Header header;
		std::string token;
		std::string filename;
		int size;
	};

	class MESSAGE_API Join {
	public:
		Join(const int version, const int errCode, const std::string& token, const std::string& accessCode);
		static Join parse(Buffer& buffer);
		void serializeTo(Buffer& buffer);

		Header header;
		std::string token;
		std::string accessCode;
		int size;
	};

	class MESSAGE_API Write {
	public:
		Write(const int version, const int errCode, const std::string& token, const COORD& cursorPos, const std::string& text);
		static Write parse(Buffer& buffer);
		void serializeTo(Buffer& buffer);

		Header header;
		std::string token;
		COORD cursorPos;
		std::string text;
		int size;
	};

	class MESSAGE_API Erase {
	public:
		Erase(const int version, const int errCode, const std::string& token, const COORD& cursorPos, const int eraseSize);
		static Erase parse(Buffer& buffer);
		void serializeTo(Buffer& buffer);

		Header header;
		std::string token;
		COORD cursorPos;
		int eraseSize;
		int size;
	};

	template<size_t N>
	int parseStrArray(Buffer& buffer, int offset, std::array<std::string, N>& arr) {
		int size = 0;
		for (auto& str : arr) {
			str = std::string{ buffer.get() + offset };
			size += str.size() + 1;
			offset += str.size() + 1;
		}
		return size;
	}

	template <size_t N>
	class ServerResponse {
	public:
		ServerResponse(const MessageType type, const int version, const int errCode, std::array<std::string, N>&& messages) :
			header(type, version, errCode),
			messages(std::move(messages)) {
			size = 3;
			for (const auto& msg : messages) {
				size += msg.size() + 1;
			}
		}
		static ServerResponse<N> parse(Buffer& buffer) {
			Header header = Header::parse(buffer);
			std::array<std::string, N> messages;
			parseStrArray(buffer, header.size, messages);
			return ServerResponse<N>(header.type, header.version, header.errCode, std::move(messages));
		}
		void serializeTo(Buffer& buffer) {
			header.serializeTo(buffer);
			for (auto& msg : messages) {
				buffer.add(&msg);
			}
		}

		Header header;
		std::array<std::string, N> messages;
		int size;
	};
}
