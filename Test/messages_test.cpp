#include "pch.h"

#include "messages.h"

TEST(WriteMessageBuildAndParseTest, MessagesTest) {
    constexpr int type = static_cast<int>(msg::MessageType::write);
    constexpr int version = 1;
    constexpr int userId = 231;
    constexpr COORD cursorPos{ 32, 54 };
    std::string text = "test";

    msg::Buffer buffer{ 128 };
    msg::Write msg{version, userId, cursorPos, text};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 15);
    EXPECT_EQ(buffer.capacity, 128);

    msg::Write parsedMsg = msg::Write::parse(buffer, 0);
    EXPECT_EQ(parsedMsg.header.version, version);
    EXPECT_EQ(parsedMsg.header.type, msg::MessageType::write);
    EXPECT_EQ(parsedMsg.header.userId, userId);
    EXPECT_EQ(parsedMsg.cursorPos.X, cursorPos.X);
    EXPECT_EQ(parsedMsg.cursorPos.Y, cursorPos.Y);
    EXPECT_EQ(parsedMsg.msg, text);
}


TEST(EraseMessageBuildAndParseTest, MessagesTest) {
    constexpr int type = static_cast<int>(msg::MessageType::erase);
    constexpr int version = 1;
    constexpr int userId = 231;
    constexpr COORD cursorPos{ 32, 54 };
    constexpr int eraseSize = 23;

    msg::Buffer buffer{ 128 };
    msg::Erase msg{version, userId, cursorPos, eraseSize};
    msg.serializeTo(buffer);
    EXPECT_EQ(buffer.size, 14);
    EXPECT_EQ(buffer.capacity, 128);

    msg::Erase parsedMsg = msg::Erase::parse(buffer, 0);
    EXPECT_EQ(parsedMsg.header.version, version);
    EXPECT_EQ(parsedMsg.header.type, msg::MessageType::erase);
    EXPECT_EQ(parsedMsg.header.userId, userId);
    EXPECT_EQ(parsedMsg.cursorPos.X, cursorPos.X);
    EXPECT_EQ(parsedMsg.cursorPos.Y, cursorPos.Y);
    EXPECT_EQ(parsedMsg.eraseSize, eraseSize);
}