#include "pch.h"

#include "repository.h"

constexpr int version = 1;
constexpr int errCode = 0;
constexpr int eraseSize = 21;
constexpr COORD cursorPos{ 19, 1 };
const std::string username = "testUsername";
const std::string password = "testPassword";
const std::string filename = "testDocFile.txt";
const std::string existingUserId = "9f3d9e66-af0e-4f66-bbbc-c714f4c40ebd";
const std::string anotherExistingUserId = "214bae8c-218d-474e-92d0-56dc344f3112";
const std::string existingUsername = "username1";
const std::string existingPassword = "password";

void fillUserDb(const std::string& dbPath) {
	std::ofstream dbFstream(dbPath, std::ostream::out);
	const std::string dbContent =
		"f11d49e1-e0c5-43cd-806d-4a9a50042696,username0,password\n"
		"9f3d9e66-af0e-4f66-bbbc-c714f4c40ebd,username1,password\n"
		"214bae8c-218d-474e-92d0-56dc344f3112,username2,password\n"
		"6566adb8-1427-4433-be38-90be6b5a5db7,username3,password\n";
	dbFstream << dbContent;
}

void fillDocDb(const std::string& dbPath) {
	std::ofstream dbFstream(dbPath, std::ostream::out);
	std::string dbContent =
		"152fbabb-b49c-4f01-a94c-cde5c628ec55,f11d49e1-e0c5-43cd-806d-4a9a50042696,0filename.txt\n"
		"586f7fbb-fcff-4d43-8031-cec2ba24003e,f11d49e1-e0c5-43cd-806d-4a9a50042696,1filename.txt\n"
		"7a8d33a8-134a-4845-95ec-f78d35c0b35f,9f3d9e66-af0e-4f66-bbbc-c714f4c40ebd,loadTest.txt\n"
		"babd37e1-e19d-463f-a5af-fcbba13296aa,6566adb8-1427-4433-be38-90be6b5a5db7,3filename.txt\n";
	dbFstream << dbContent;
}

void createDocFileForWrite(const std::string& name) {
	std::ofstream file(name, std::ostream::out);
	if (file) {
		file << "This is test for write\nIt will be updated during some tests and then deleted\n";
	}
}

template<typename MESSAGE, typename RESPONSE, typename... Args>
std::pair<RESPONSE, ResponseType> processMsg(Repository& repo, Args&... args) {
	msg::Buffer buffer{128};
	MESSAGE msg{ args... };
	msg.serializeTo(buffer);
	auto [outBuff, dst] = repo.process(buffer);
	return std::pair<RESPONSE, ResponseType>( RESPONSE::parse(outBuff), dst );
}

TEST(RepositoryTests, HappyRegisterTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);

	auto [out, dst] = processMsg<msg::Register, msg::ServerResponse<1>>(
		repository, version, errCode, username, password
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::registration);
	EXPECT_EQ(out.header.errCode, 0);
	EXPECT_FALSE(std::remove(userDbPath.c_str()));
}

TEST(RepositoryTests, UserExistsErrorRegisterTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);

	auto [out, dst] = processMsg<msg::Register, msg::ServerResponse<1>>(
		repository, version, errCode, existingUsername, password
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::error);
	EXPECT_EQ(out.header.errCode, 1);
	EXPECT_FALSE(std::remove(userDbPath.c_str()));
}

TEST(RepositoryTests, HappyLoginTest) {
	logs::Logger logger("test.log");
	Repository repository{ "ReadUserTest.csv", "ReadDocTest.csv", logger};

	auto [out, dst] = processMsg<msg::Login, msg::ServerResponse<1>>(
		repository, version, errCode, existingUsername, existingPassword
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::login);
	EXPECT_EQ(out.header.errCode, 0);
	EXPECT_EQ(out.messages[0], existingUserId);
}

TEST(RepositoryTests, InvalidUsernameLoginTest) {
	logs::Logger logger("test.log");
	Repository repository{ "ReadUserTest.csv", "ReadDocTest.csv", logger };

	auto [out, dst] = processMsg<msg::Login, msg::ServerResponse<1>>(
		repository, version, errCode, username, existingPassword
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::error);
	EXPECT_EQ(out.header.errCode, 1);
}

TEST(RepositoryTests, InvalidPasswordLoginTest) {
	logs::Logger logger("test.log");
	Repository repository{ "ReadUserTest.csv", "ReadDocTest.csv", logger };

	auto [out, dst] = processMsg<msg::Login, msg::ServerResponse<1>>(
		repository, version, errCode, existingUsername, password
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::error);
	EXPECT_EQ(out.header.errCode, 1);
}

TEST(RepositoryTests, HappyCreateDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [out, dst] = processMsg<msg::Create, msg::ServerResponse<1>>(
		repository, version, errCode, existingUserId, filename
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::create);
	EXPECT_EQ(out.header.errCode, 0);

	const std::string realFileName = existingUserId + "-" + filename;
	std::ifstream file(realFileName);
	EXPECT_TRUE(file);
	file.close();
	EXPECT_FALSE(std::remove(realFileName.c_str()));
	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, WrongUserIdCreateDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [out, dst] = processMsg<msg::Create, msg::ServerResponse<1>>(
		repository, version, errCode, "notexistinguserid", filename
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::error);
	EXPECT_EQ(out.header.errCode, 1);
	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, HappyLoadDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [out, dst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "loadTest.txt"
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::load);
	EXPECT_EQ(out.header.errCode, 0);
	EXPECT_EQ(out.messages[0], "Some random text,\neverything should work!\n");

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, WrongUserIdLoadDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [out, dst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, "nonexistinguserid", "loadTest.txt"
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::error);
	EXPECT_EQ(out.header.errCode, 1);

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, WrongFilenameLoadDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [out, dst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "wrongfilename.txt"
	);
	EXPECT_EQ(dst, ResponseType::unicast);
	EXPECT_EQ(out.header.type, msg::MessageType::error);
	EXPECT_EQ(out.header.errCode, 1);

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, HappyJoinDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [loadOut, loadDst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "loadTest.txt"
	);
	auto [joinOut, joinDst] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, anotherExistingUserId, loadOut.messages[1]
	);
	EXPECT_EQ(joinDst, ResponseType::unicast);
	EXPECT_EQ(joinOut.header.type, msg::MessageType::join);
	EXPECT_EQ(joinOut.header.errCode, 0);
	EXPECT_EQ(joinOut.messages[0], "Some random text,\neverything should work!\n");

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, WrongUserIdJoinDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [loadOut, loadDst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "loadTest.txt"
	);
	auto [joinOut, joinDst] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, "nonexistinguserid", loadOut.messages[1]
	);
	EXPECT_EQ(joinDst, ResponseType::unicast);
	EXPECT_EQ(joinOut.header.type, msg::MessageType::error);
	EXPECT_EQ(joinOut.header.errCode, 1);

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, WrongAccessCodeJoinDocTest) {
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);

	auto [loadOut, loadDst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "loadTest.txt"
	);
	auto [joinOut, joinDst] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, anotherExistingUserId, "nonexistingaccesscode"
	);
	EXPECT_EQ(joinDst, ResponseType::unicast);
	EXPECT_EQ(joinOut.header.type, msg::MessageType::error);
	EXPECT_EQ(joinOut.header.errCode, 1);

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
}

TEST(RepositoryTests, OriginalUserWriteToDocTest) {
	const std::string docFileForWrite = existingUserId + "-" + "test.txt";
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);
	createDocFileForWrite(docFileForWrite);

	auto [loadOut, loadDst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "test.txt"
	);
	auto [writeOut, writeDst] = processMsg<msg::Write, msg::Write>(
		repository, version, errCode, existingUserId, cursorPos, "by unit test "
	);
	EXPECT_EQ(writeDst, ResponseType::broadcast);
	EXPECT_EQ(writeOut.header.type, msg::MessageType::write);
	EXPECT_EQ(writeOut.header.errCode, 0);
	EXPECT_EQ(writeOut.cursorPos.X, cursorPos.X);
	EXPECT_EQ(writeOut.cursorPos.Y, cursorPos.Y);
	EXPECT_EQ(writeOut.text, "by unit test ");

	auto [joinOut, joinDst] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, anotherExistingUserId, loadOut.messages[1]
	);
	EXPECT_EQ(joinOut.messages[0], "This is test for write\nIt will be updated by unit test during some tests and then deleted\n");

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
	EXPECT_FALSE(std::remove(docFileForWrite.c_str()));
}


TEST(RepositoryTests, JoinedUserWriteToDocTest) {
	const std::string docFileForWrite = existingUserId + "-" + "test.txt";
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);
	createDocFileForWrite(docFileForWrite);

	auto [loadOut, loadDst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "test.txt"
	);
	auto [joinOut, joinDst] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, anotherExistingUserId, loadOut.messages[1]
	);
	auto [writeOut, writeDst] = processMsg<msg::Write, msg::Write>(
		repository, version, errCode, anotherExistingUserId, cursorPos, "by unit test "
	);
	EXPECT_EQ(writeDst, ResponseType::broadcast);
	EXPECT_EQ(writeOut.header.type, msg::MessageType::write);
	EXPECT_EQ(writeOut.header.errCode, 0);
	EXPECT_EQ(writeOut.cursorPos.X, cursorPos.X);
	EXPECT_EQ(writeOut.cursorPos.Y, cursorPos.Y);
	EXPECT_EQ(writeOut.text, "by unit test ");
	auto [joinOut2, joinDst2] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, existingUserId, loadOut.messages[1]
	);
	EXPECT_EQ(joinOut2.messages[0], "This is test for write\nIt will be updated by unit test during some tests and then deleted\n");

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
	EXPECT_FALSE(std::remove(docFileForWrite.c_str()));
}

TEST(RepositoryTests, OriginalUserEraseFromDocTest) {
	const std::string docFileForWrite = existingUserId + "-" + "test.txt";
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);
	createDocFileForWrite(docFileForWrite);

	auto [loadOut, loadDst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "test.txt"
	);
	auto [eraseOut, eraseDst] = processMsg<msg::Erase, msg::Erase>(
		repository, version, errCode, existingUserId, cursorPos, eraseSize
	);
	EXPECT_EQ(eraseDst, ResponseType::broadcast);
	EXPECT_EQ(eraseOut.header.type, msg::MessageType::erase);
	EXPECT_EQ(eraseOut.header.errCode, 0);
	EXPECT_EQ(eraseOut.cursorPos.X, cursorPos.X);
	EXPECT_EQ(eraseOut.cursorPos.Y, cursorPos.Y);
	EXPECT_EQ(eraseOut.eraseSize, eraseSize);

	auto [joinOut, joinDst] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, anotherExistingUserId, loadOut.messages[1]
	);
	EXPECT_EQ(joinOut.messages[0], "This is test for writduring some tests and then deleted\n");

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
	EXPECT_FALSE(std::remove(docFileForWrite.c_str()));
}


TEST(RepositoryTests, JoinedUserEraseFromDocTest) {
	const std::string docFileForWrite = existingUserId + "-" + "test.txt";
	const std::string name = testing::UnitTest::GetInstance()->current_test_info()->name();
	const std::string userDbPath = name + "Users.csv";
	const std::string docDbPath = name + "Docs.csv";
	logs::Logger logger("test.log");
	Repository repository{ userDbPath, docDbPath, logger };
	fillUserDb(userDbPath);
	fillDocDb(docDbPath);
	createDocFileForWrite(docFileForWrite);

	auto [loadOut, loadDst] = processMsg<msg::Load, msg::ServerResponse<2>>(
		repository, version, errCode, existingUserId, "test.txt"
	);
	auto [joinOut, joinDst] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, anotherExistingUserId, loadOut.messages[1]
	);
	auto [eraseOut, eraseDst] = processMsg<msg::Erase, msg::Erase>(
		repository, version, errCode, anotherExistingUserId, cursorPos, eraseSize
	);
	EXPECT_EQ(eraseDst, ResponseType::broadcast);
	EXPECT_EQ(eraseOut.header.type, msg::MessageType::erase);
	EXPECT_EQ(eraseOut.header.errCode, 0);
	EXPECT_EQ(eraseOut.cursorPos.X, cursorPos.X);
	EXPECT_EQ(eraseOut.cursorPos.Y, cursorPos.Y);
	EXPECT_EQ(eraseOut.eraseSize, eraseSize);
	auto [joinOut2, joinDst2] = processMsg<msg::Join, msg::ServerResponse<1>>(
		repository, version, errCode, existingUserId, loadOut.messages[1]
	);
	EXPECT_EQ(joinOut2.messages[0], "This is test for writduring some tests and then deleted\n");

	EXPECT_FALSE(std::remove(userDbPath.c_str()));
	EXPECT_FALSE(std::remove(docDbPath.c_str()));
	EXPECT_FALSE(std::remove(docFileForWrite.c_str()));
}