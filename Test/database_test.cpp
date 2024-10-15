#include "pch.h"
#include <string>

#include "database.h"

TEST(CRUDUserTest, DatabaseTests) {
	const std::string username = "username";
	const std::string updatedUsername = "updatedUsername";
	const std::string password = "password";
	std::string root = ".";
	logs::Logger logger("test.log");
	db::Database db{ root, logger};

	db::UserData userData{ username, password };
	std::string userId = db.createUser(userData);
	EXPECT_FALSE(userId.empty());

	db::UserData readUserData = db.readUser(userId);
	EXPECT_EQ(readUserData.uuid, userId);
	EXPECT_EQ(readUserData.username, username);
	EXPECT_EQ(readUserData.password, password);

	readUserData.username = updatedUsername;
	EXPECT_TRUE(db.updateUser(readUserData));

	db::UserData updatedUserData = db.readUser(userId);
	EXPECT_EQ(readUserData.uuid, userId);
	EXPECT_EQ(readUserData.username, updatedUsername);
	EXPECT_EQ(readUserData.password, password);

	EXPECT_TRUE(db.deleteUser(userId));
	EXPECT_FALSE(db.deleteUser(userId));
}

TEST(CRUDDocTest, DatabaseTests) {
	const std::string filename = "filename.txt";
	std::string root = ".";
	logs::Logger logger("test.log");
	db::Database db{ root, logger};

	db::DocData docData{ filename };
	std::string docId = db.createDoc(docData);
	EXPECT_FALSE(docId.empty());
	db::DocData readDocData = db.readDoc(docId);
	EXPECT_EQ(readDocData.uuid, docId);
	EXPECT_EQ(readDocData.filename, filename);
}