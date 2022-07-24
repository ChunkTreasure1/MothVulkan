#pragma once

#include <Lamp/Core/Base.h>
#include <stack>

class EditorCommand;
class EditorCommandStack
{
public:
	void Push(Ref<EditorCommand> cmd);
	void Undo();

private:
	std::stack<Ref<EditorCommand>> m_commandStack;
};