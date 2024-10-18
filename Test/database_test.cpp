#include "pch.h"
#include <string>

#include "database.h"

const std::string userId = "randomId";

TEST(DatabaseTests, CreateUserTest) {
	std::string dbPath = "CreateUserTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	const std::string username = "username";
	const std::string password = "password";
	
	db::User userData{ username, password };
	std::string uuid = db.create(userData);
	EXPECT_FALSE(uuid.empty());
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, CreateUserWithTheSameUuidTest) {
	std::string dbPath = "CreateUserWithTheSameUuidTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	const std::string username = "username";
	const std::string password = "password";

	db::User user{ username, password };
	std::string uuid = db.create(user);
	EXPECT_FALSE(uuid.empty());

	db::User nextUser{uuid, user};
	uuid = db.create(nextUser);
	EXPECT_TRUE(uuid.empty());
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, CreateUserWithTheSameUsernameTest) {
	std::string dbPath = "CreateUserWithTheSameUsernameTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	const std::string username = "username";
	const std::string password = "password";

	db::User user{ username, password };
	std::string uuid = db.create(user);
	EXPECT_FALSE(uuid.empty());

	db::User nextUser{username, password};
	uuid = db.create(user);
	EXPECT_TRUE(uuid.empty());
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, CreateMultipleUsersTest) {
	std::string dbPath = "CreateMultipleUsersTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	const std::string baseUsername = "username";
	const std::string password = "password";

	for (int i = 0; i < 10; i++) {
		db::User user{ baseUsername + std::to_string(i), password };
		std::string uuid = db.create(user);
		EXPECT_FALSE(uuid.empty());
	}
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, CreateDocTest) {
	std::string dbPath = "CreateDocTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::Doc> db{dbPath, logger};

	const std::string filename = "filename.txt";
	db::Doc doc{userId, filename };
	std::string uuid = db.create(doc);
	EXPECT_FALSE(uuid.empty());
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, CreateDocWithTheSameUuidTest) {
	std::string dbPath = "CreateDocWithTheSameUuidTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::Doc> db{dbPath, logger};

	const std::string filename = "filename.txt";
	db::Doc doc{ userId, filename };
	std::string uuid = db.create(doc);
	EXPECT_FALSE(uuid.empty());

	db::Doc nextDoc{uuid, doc};
	uuid = db.create(nextDoc);
	EXPECT_TRUE(uuid.empty());
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, CreateMultipleDocsTest) {
	std::string dbPath = "CreateMultipleDocsTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::Doc> db{dbPath, logger};

	const std::string baseFilename = "filename.txt";
	for (int i = 0; i < 10; i++) {
		db::Doc doc{ userId, std::to_string(i) + baseFilename };
		std::string uuid = db.create(doc);
		EXPECT_FALSE(uuid.empty());
	}
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, ReadUserTest) {
	std::string uuid = "0081aa72-f4a9-4835-990c-50dbfdb0279c";
	std::string dbPath = "ReadUserTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	db::User user = db.read(uuid);
	EXPECT_EQ(user.uuid, uuid);
	EXPECT_EQ(user.username, "username5");
	EXPECT_EQ(user.password, "password");
}

TEST(DatabaseTests, ReadUserWithUsernameTest) {
	std::string username = "username7";
	std::string uuid = "17a02b66-044a-4037-80a3-f5fdce4456be";
	std::string password = "password";

	std::string dbPath = "ReadUserTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	db::User user = db.readWithAttribute(username, 1);
	EXPECT_EQ(user.uuid, uuid);
	EXPECT_EQ(user.username, username);
	EXPECT_EQ(user.password, password);
}

TEST(DatabaseTests, ReadDocTest) {
	std::string uuid = "f81d855c-b568-4731-bbd0-330e090aa6ce";
	std::string dbPath = "ReadDocTest.csv";
	logs::Logger logger("test.log");
	db::Database<db::Doc> db{dbPath, logger};

	db::Doc doc = db.read(uuid);
	EXPECT_EQ(doc.uuid, uuid);
	EXPECT_EQ(doc.userId, userId);
	EXPECT_EQ(doc.filename, "5filename.txt");
}

TEST(DatabaseTests, UpdateUserTest) {
	std::string dbPath = "UpdateUserTest.csv";
	std::string uuid = "9f3d9e66-af0e-4f66-bbbc-c714f4c40ebd";
	std::string updatedUsername = "updatedUsername";

	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	std::ofstream dbFstream(dbPath, std::ostream::out);
	std::string dbContent =
		"f11d49e1-e0c5-43cd-806d-4a9a50042696,username0,password\n"
		"9f3d9e66-af0e-4f66-bbbc-c714f4c40ebd,username1,password\n"
		"214bae8c-218d-474e-92d0-56dc344f3112,username2,password\n"
		"6566adb8-1427-4433-be38-90be6b5a5db7,username3,password\n";
	dbFstream << dbContent;
	dbFstream.close();

	db::User user = db.read(uuid);
	std::vector<std::string> row = { user.uuid, updatedUsername, user.password };
	db::User newUser{row};
	bool updated = db.update(newUser);
	db::User updatedUser = db.read(uuid);

	EXPECT_TRUE(updated);
	EXPECT_EQ(updatedUser.uuid, uuid);
	EXPECT_EQ(updatedUser.uuid, user.uuid);
	EXPECT_EQ(updatedUser.username, updatedUsername);
	EXPECT_EQ(updatedUser.password, user.password);
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, UpdateDocTest) {
	std::string dbPath = "UpdateDocTest.csv";
	std::string uuid = "babd37e1-e19d-463f-a5af-fcbba13296aa";
	std::string updatedFilename = "updatedFilename.txt";

	logs::Logger logger("test.log");
	db::Database<db::Doc> db{dbPath, logger};

	std::ofstream dbFstream(dbPath, std::ostream::out);
	std::string dbContent =
		"152fbabb-b49c-4f01-a94c-cde5c628ec55,randomId,0filename.txt\n"
		"586f7fbb-fcff-4d43-8031-cec2ba24003e,randomId,1filename.txt\n"
		"7a8d33a8-134a-4845-95ec-f78d35c0b35f,randomId,2filename.txt\n"
		"babd37e1-e19d-463f-a5af-fcbba13296aa,randomId,3filename.txt\n";
	dbFstream << dbContent;
	dbFstream.close();

	db::Doc doc = db.read(uuid);
	std::vector<std::string> row = { doc.uuid, doc.userId, updatedFilename };
	db::Doc newDoc{row};
	bool updated = db.update(newDoc);
	db::Doc updatedDoc = db.read(uuid);

	EXPECT_TRUE(updated);
	EXPECT_EQ(updatedDoc.uuid, uuid);
	EXPECT_EQ(updatedDoc.uuid, doc.uuid);
	EXPECT_EQ(updatedDoc.filename, updatedFilename);
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, DeleteUserTest) {
	std::string dbPath = "DeleteUserTest.csv";
	std::string uuid = "9f3d9e66-af0e-4f66-bbbc-c714f4c40ebd";

	logs::Logger logger("test.log");
	db::Database<db::User> db{dbPath, logger};

	std::ofstream dbFstream(dbPath, std::ostream::out);
	std::string dbContent =
		"f11d49e1-e0c5-43cd-806d-4a9a50042696,username0,password\n"
		"9f3d9e66-af0e-4f66-bbbc-c714f4c40ebd,username1,password\n"
		"214bae8c-218d-474e-92d0-56dc344f3112,username2,password\n"
		"6566adb8-1427-4433-be38-90be6b5a5db7,username3,password\n";
	dbFstream << dbContent;
	dbFstream.close();

	bool deleted = db.erase(uuid);
	db::User user = db.read(uuid);

	EXPECT_TRUE(deleted);
	EXPECT_EQ(user.uuid, "");
	EXPECT_EQ(user.username, "");
	EXPECT_EQ(user.password, "");
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}

TEST(DatabaseTests, DeleteDocTest) {
	std::string dbPath = "DeleteDocTest.csv";
	std::string uuid = "babd37e1-e19d-463f-a5af-fcbba13296aa";

	logs::Logger logger("test.log");
	db::Database<db::Doc> db{dbPath, logger};

	std::ofstream dbFstream(dbPath, std::ostream::out);
	std::string dbContent =
		"152fbabb-b49c-4f01-a94c-cde5c628ec55,0filename.txt\n"
		"586f7fbb-fcff-4d43-8031-cec2ba24003e,1filename.txt\n"
		"7a8d33a8-134a-4845-95ec-f78d35c0b35f,2filename.txt\n"
		"babd37e1-e19d-463f-a5af-fcbba13296aa,3filename.txt\n";
	dbFstream << dbContent;
	dbFstream.close();

	bool deleted = db.erase(uuid);
	db::Doc doc = db.read(uuid);

	EXPECT_TRUE(deleted);
	EXPECT_EQ(doc.uuid, "");
	EXPECT_EQ(doc.filename, "");
	EXPECT_FALSE(std::remove(dbPath.c_str()));
}
