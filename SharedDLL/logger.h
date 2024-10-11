#pragma once
#include <fstream>
#include <sstream>

#ifdef SHAREDDLL_EXPORTS
#define LOGGER_API __declspec(dllexport)
#else
#define LOGGER_API __declspec(dllimport)
#endif

class LOGGER_API Logger {
public:
	Logger(std::string logFilePath);
	~Logger();
	void log(const std::string& message);

private:
	std::ofstream file;
};
