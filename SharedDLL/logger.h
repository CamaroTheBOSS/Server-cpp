#pragma once
#include <fstream>
#include <sstream>
#include <map>

#pragma push_macro("ERROR")
#undef ERROR

#ifdef SHAREDDLL_EXPORTS
#define LOGGER_API __declspec(dllexport)
#else
#define LOGGER_API __declspec(dllimport)
#endif

namespace logs {
	enum class Level { ERROR, INFO, DEBUG};

	std::string LOGGER_API lvlToStr(Level lvl);

	class LOGGER_API Logger {
	public:
		Logger(std::string logFilePath);
		~Logger();

		template<typename... Args>
		void log(Level lvl, const Args... args) {
			time_t timestamp;
			time(&timestamp);
			std::stringstream stream;
			char timeBuffer[100];
			if (ctime_s(timeBuffer, 100, &timestamp)) {
				return;
			}
			stream << "[" << timeBuffer << "] " << lvlToStr(lvl);
			([&] {
				stream << args;
			} (), ...);
			file << stream.str() << "\n" << std::flush;
		}


	private:
		std::ofstream file;
	};
}

#pragma pop_macro("ERROR")
