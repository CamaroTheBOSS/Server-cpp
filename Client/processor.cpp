#include "processor.h"
#include <ctime>

#pragma push_macro("ERROR")
#undef ERROR

Processor::Processor(Document& doc, TerminalManager& terminal, logs::Logger& logger, std::string& userId) :
	doc(doc),
	terminal(terminal),
	logger(logger),
	userId{ userId } {}

std::pair<std::string, int> Processor::waitForResponse() {
	time_t currTime, timeoutTime;
	time(&timeoutTime);
	timeoutTime += 2;
	while (!responseReady) {
		time(&currTime);
		if (currTime >= timeoutTime) {
			return { "timeout", 1 };
		}
	}
	responseReady = false;
	return { response, errCode };
}

void Processor::process(msg::Buffer& buffer) {
	auto header = msg::Header::parse(buffer);
	std::pair<std::string, int> responseAndErrCode;
	switch (header.type) {
	case msg::MessageType::write:
		responseAndErrCode = processWriteMsg(buffer);
		break;
	case msg::MessageType::erase:
		responseAndErrCode = processEraseMsg(buffer);
		break;
	case msg::MessageType::registration:
		responseAndErrCode = processRegisterMsg(buffer);
		break;
	case msg::MessageType::login:
		responseAndErrCode = processLoginMsg(buffer);
		break;
	case msg::MessageType::create:
		responseAndErrCode = processCreateMsg(buffer);
		break;
	case msg::MessageType::load:
		responseAndErrCode = processLoadMsg(buffer);
		break;
	case msg::MessageType::join:
		responseAndErrCode = processJoinMsg(buffer);
		break;
	case msg::MessageType::error:
		responseAndErrCode = processErrorMsg(buffer);
		break;
	default:
		logger.log(logs::Level::ERROR, "Unknown header type in incoming message");
	}
	response = responseAndErrCode.first;
	errCode = responseAndErrCode.second;
	responseReady = true;
}

std::pair<std::string, int> Processor::processWriteMsg(msg::Buffer& buffer) {
	auto msg = msg::Write::parse(buffer);
	COORD docCursorPos = doc.getCursorPos();
	if (doc.setCursorPos(msg.cursorPos)) {
		doc.write(msg.text);
		if (msg.token != userId) {
			doc.setCursorPos(docCursorPos);
			if (msg.cursorPos.Y == docCursorPos.Y && msg.cursorPos.X <= docCursorPos.X) {
				doc.moveCursorRight();
			}
		}
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] wrote '", msg.text, "'");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on write msg!");
	}
	terminal.render(doc);
	return { "", msg.header.errCode };
}

std::pair<std::string, int> Processor::processEraseMsg(msg::Buffer& buffer) {
	auto msg = msg::Erase::parse(buffer);
	COORD docCursorPos = doc.getCursorPos();
	if (doc.setCursorPos(msg.cursorPos)) {
		for (int i = 0; i < msg.eraseSize; i++) {
			doc.erase();
		}
		if (msg.token != userId) {
			doc.setCursorPos(docCursorPos);
			if (msg.cursorPos.Y == docCursorPos.Y && msg.cursorPos.X <= docCursorPos.X) {
				doc.moveCursorLeft();
			}
		}
		logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] erased ", msg.eraseSize, " letters");
	}
	else {
		logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on erase msg!");
	}
	terminal.render(doc);
	return { "", msg.header.errCode };
}

std::pair<std::string, int> Processor::processErrorMsg(msg::Buffer& buffer) {
	auto msg = msg::ServerResponse<1>::parse(buffer);
	return { msg.messages[0], msg.header.errCode };
}

std::pair<std::string, int> Processor::processRegisterMsg(msg::Buffer& buffer) {
	auto msg = msg::ServerResponse<1>::parse(buffer);
	return { msg.messages[0], msg.header.errCode };
}

std::pair<std::string, int> Processor::processLoginMsg(msg::Buffer& buffer) {
	auto msg = msg::ServerResponse<1>::parse(buffer);
	userId = msg.messages[0];
	return { "Login successful", msg.header.errCode};
}

std::pair<std::string, int> Processor::processCreateMsg(msg::Buffer& buffer) {
	auto msg = msg::ServerResponse<1>::parse(buffer);
	return { msg.messages[0], msg.header.errCode };
}

std::pair<std::string, int> Processor::processLoadMsg(msg::Buffer& buffer) {
	auto msg = msg::ServerResponse<2>::parse(buffer);
	doc.setText(msg.messages[0]);
	return { msg.messages[1], msg.header.errCode };
}

std::pair<std::string, int> Processor::processJoinMsg(msg::Buffer& buffer) {
	auto msg = msg::ServerResponse<1>::parse(buffer);
	doc.setText(msg.messages[0]);
	return { msg.messages[0], msg.header.errCode };
}

#pragma pop_macro("ERROR")
