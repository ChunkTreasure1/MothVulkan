#include <sbpch.h>

#include "EditorCommandStack.h"
#include "EditorCommand.h"	

void EditorCommandStack::Push(Ref<EditorCommand> cmd)
{
	m_commandStack.emplace(cmd);
}

void EditorCommandStack::Undo()
{
	if (m_commandStack.empty())
	{
		return;
	}

	m_commandStack.top()->Undo();
	m_commandStack.pop();
}