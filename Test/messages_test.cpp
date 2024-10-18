#include "pch.h"
#include <array>

#include "messages.h"

constexpr int version = 1;
constexpr int errCode = 12;
constexpr int eraseSize = 40;
constexpr COORD cursorPos{ 32, 54 };
const std::string username = "username";
const std::string password = "password";
const std::string token = "token";
const std::string filename = "filename.txt";
const std::string accessCode = "C7JKFN";
const std::string text = "mea culpa";

TEST(MessagesTest, HeaderSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Header msg{msg::MessageType::write, version, errCode};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 3);
    EXPECT_EQ(buffer.capacity, 128);

    auto parsed = msg::Header::parse(buffer);
    EXPECT_EQ(parsed.version, version);
    EXPECT_EQ(parsed.type, msg::MessageType::write);
    EXPECT_EQ(parsed.errCode, errCode);
}

TEST(MessagesTest, RegisterSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Register msg{version, errCode, username, password};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 21);
    EXPECT_EQ(buffer.capacity, 128);

    auto parsed = msg::Register::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::registration);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.username, username);
    EXPECT_EQ(parsed.password, password);
}

TEST(MessagesTest, LoginSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Login msg{version, errCode, username, password};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 21);
    EXPECT_EQ(buffer.capacity, 128);

    auto parsed = msg::Login::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::login);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.username, username);
    EXPECT_EQ(parsed.password, password);
}

TEST(MessagesTest, CreateSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Create msg{version, errCode, token, filename};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 22);
    EXPECT_EQ(buffer.capacity, 128);

    auto parsed = msg::Create::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::create);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.token, token);
    EXPECT_EQ(parsed.filename, filename);
}

TEST(MessagesTest, LoadSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Load msg{version, errCode, token, filename};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 22);
    EXPECT_EQ(buffer.capacity, 128);

    auto parsed = msg::Load::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::load);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.token, token);
    EXPECT_EQ(parsed.filename, filename);
}

TEST(MessagesTest, JoinSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Join msg{version, errCode, token, accessCode};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 16);
    EXPECT_EQ(buffer.capacity, 128);

    auto parsed = msg::Join::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::join);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.token, token);
    EXPECT_EQ(parsed.accessCode, accessCode);
}

TEST(MessagesTest, WriteSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Write msg{version, errCode, token, cursorPos, text};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 23);
    EXPECT_EQ(buffer.capacity, 128);

    msg::Write parsed = msg::Write::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::write);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.token, token);
    EXPECT_EQ(parsed.cursorPos.X, cursorPos.X);
    EXPECT_EQ(parsed.cursorPos.Y, cursorPos.Y);
    EXPECT_EQ(parsed.text, text);
}

TEST(MessagesTest, EraseSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    msg::Erase msg{version, errCode, token, cursorPos, eraseSize};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 17);
    EXPECT_EQ(buffer.capacity, 128);

    msg::Erase parsed = msg::Erase::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::erase);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.token, token);
    EXPECT_EQ(parsed.cursorPos.X, cursorPos.X);
    EXPECT_EQ(parsed.cursorPos.Y, cursorPos.Y);
    EXPECT_EQ(parsed.eraseSize, eraseSize);
}

TEST(MessagesTest, ServerResponseSerializeAndParseTest) {
    msg::Buffer buffer{ 128 };
    std::array<std::string, 3> messages = {"msg1", "msg2", "msg3"};
    msg::ServerResponse<3> msg{msg::MessageType::error, version, errCode, std::move(messages)};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 18);
    EXPECT_EQ(buffer.capacity, 128);
    
    auto parsed = msg::ServerResponse<3>::parse(buffer);
    EXPECT_EQ(parsed.header.version, version);
    EXPECT_EQ(parsed.header.type, msg::MessageType::error);
    EXPECT_EQ(parsed.header.errCode, errCode);
    EXPECT_EQ(parsed.messages[0], "msg1");
    EXPECT_EQ(parsed.messages[1], "msg2");
    EXPECT_EQ(parsed.messages[2], "msg3");
}
