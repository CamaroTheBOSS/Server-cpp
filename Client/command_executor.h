#include <string>
#include <vector>

#include "tcp_client.h"

class CommandExecutor {
public:
	enum class CommandType {registration, login, create, load, join, help, exit, err};

	CommandExecutor(Client& tcpClient);
	std::pair<std::string, int> processCommand(const std::string& command);
private:
	std::vector<std::string> parse(const std::string& command);
	std::vector<std::string> parseCommand(const std::string& command);
	CommandType checkCommandType(const std::vector<std::string>& args);
	Client& tcpClient;
	std::string errMsg;
};