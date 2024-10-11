#include "pch.h"
#include <ctime>
#include "logger.h"

Logger::Logger(std::string logFilePath):
	file(logFilePath, std::ofstream::out | std::ofstream::trunc) {}

Logger::~Logger() {
	file.close();
}

void Logger::log(const std::string& message) {
	time_t timestamp;
	time(&timestamp);
	std::stringstream stream;
	char timeBuffer[100];
	if (ctime_s(timeBuffer, 100, &timestamp)) {
		return;
	}
	stream << "[" << timeBuffer << "] " << message << '\n';
	file << stream.str() << std::flush;
}