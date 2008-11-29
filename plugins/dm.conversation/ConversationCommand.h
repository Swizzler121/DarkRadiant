#ifndef CONVERSATION_COMMAND_H_
#define CONVERSATION_COMMAND_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

namespace conversation {

/**
 * Data object representing a single ConversationCommand.
 */
class ConversationCommand
{
public:
	// What kind of command this is
	std::string type;

	// Which actor should perform this command.
	int actor;

	// whether the command must be finished before the next command can start
	bool waitUntilFinished;

	// The numbered arguments
	std::map<int, std::string> arguments;
	
	// Constructor
	ConversationCommand() :
		type("WaitSeconds"),
		actor(-1),
		waitUntilFinished(true)
	{}
};
typedef boost::shared_ptr<ConversationCommand> ConversationCommandPtr;

} // namespace conversation

#endif /* CONVERSATION_COMMAND_H_ */
