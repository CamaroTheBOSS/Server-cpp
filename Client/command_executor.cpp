#include "command_executor.h"
#include "messages.h"

CommandExecutor::CommandExecutor(Client& tcpClient):
	tcpClient(tcpClient) {}

std::pair<std::string, int> CommandExecutor::processCommand(const std::string& command) {
	if (command.empty()) {
		return {"Empty command", 1};
	}
	auto commandVec = parseCommand(command);
	auto commandType = checkCommandType(commandVec);
	std::string msg;
	switch (commandType) {
	case CommandType::registration:
		tcpClient.sendMsg<msg::Register>(1, 0, commandVec[1], commandVec[2]);
		break;
	case CommandType::login:
		tcpClient.sendMsg<msg::Login>(1, 0, commandVec[1], commandVec[2]);
		break;
	case CommandType::create:
		tcpClient.sendMsg<msg::Create>(1, 0, tcpClient.getUserId(), commandVec[1]);
		break;
	case CommandType::load:
		tcpClient.sendMsg<msg::Load>(1, 0, tcpClient.getUserId(), commandVec[1]);
		break;
	case CommandType::join:
		tcpClient.sendMsg<msg::Join>(1, 0, tcpClient.getUserId(), commandVec[1]);
		break;
	case CommandType::help:
		return {
			"register <username> <password>\n"
			"login <username> <password>\n"
			"create <filename>\n"
			"load <filename>\n"
			"join <access_code>\n", 0 };
	case CommandType::err:
		msg = errMsg;
		errMsg = "";
		return { msg, 0 };
	case CommandType::exit:
		return { "", -1 };
	}
	auto responseAndCode = tcpClient.waitForResponse();
	return responseAndCode;
}

std::vector<std::string> CommandExecutor::parse(const std::string& command) {
	std::vector<std::string> parsedCommand = parseCommand(command);
	if (parsedCommand.size() == 0) {
		return {};
	}
	auto& type = parsedCommand[0];
	return {};
}

std::vector<std::string> CommandExecutor::parseCommand(const std::string& command) {
	std::vector<std::string> parsedCommand;
	int offset = 0;
	int delimiterPos = 0;
	while ((delimiterPos = command.find(' ', offset)) != std::string::npos) {
		std::string arg = command.substr(offset, delimiterPos - offset);
		if (!arg.empty()) {
			parsedCommand.push_back(arg);
		}
		offset = delimiterPos + 1;
	}
	parsedCommand.emplace_back(command.substr(offset, command.size() - offset));
	return parsedCommand;
}

CommandExecutor::CommandType CommandExecutor::checkCommandType(const std::vector<std::string>& args) {
	auto typeStr = args[0];
	auto commandType = CommandType::err;
	int desiredSize = 0;
	if (typeStr == "register") {
		desiredSize = 3;
		commandType = CommandType::registration;
	}
	else if (typeStr == "login") {
		desiredSize = 3;
		commandType = CommandType::login;
	}
	else if (typeStr == "create") {
		desiredSize = 2;
		commandType = CommandType::create;
	}
	else if (typeStr == "load") {
		desiredSize = 2;
		commandType = CommandType::load;
	}
	else if (typeStr == "join") {
		desiredSize = 2;
		commandType = CommandType::join;
	}
	else if (typeStr == "help") {
		desiredSize = 1;
		commandType = CommandType::help;
	}
	else if (typeStr == "exit") {
		desiredSize = 1;
		commandType = CommandType::exit;
	}
	if (desiredSize == 0) {
		errMsg = "unrecognized command '" + typeStr + "'";
		return CommandType::err;
	}
	if (args.size() != desiredSize) {
		errMsg = "command '" + typeStr + "' takes " + std::to_string(desiredSize - 1) + " arguments, but " + std::to_string(args.size() - 1) + " were given";
		return CommandType::err;
	}
	return commandType;
}