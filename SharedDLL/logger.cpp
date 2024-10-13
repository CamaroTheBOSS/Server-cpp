#include "pch.h"
#include <ctime>
#include "logger.h"

#pragma push_macro("ERROR")
#undef ERROR

namespace logs {

	std::string lvlToStr(Level lvl) {
		switch (lvl) {
		case Level::ERROR:
			return "ERROR ";
		case Level::INFO:
			return "INFO ";
		case Level::DEBUG:
			return "DEBUG ";
		}
		return "UNDEFINED";
	}

	Logger::Logger(std::string logFilePath):
		file(logFilePath, std::ofstream::out | std::ofstream::trunc) {}

	Logger::~Logger() {
		file.close();
}

}

#pragma pop_macro("ERROR")