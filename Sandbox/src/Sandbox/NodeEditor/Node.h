#pragma once

#include <imgui_node_editor.h>

#include <string>
#include <vector>

namespace ed = ax::NodeEditor;

enum class PinType
{
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate
};

enum class PinMode
{
	Output,
	Input
};

struct Node;

struct Pin
{
	ed::PinId id;
	Node* node;
	std::string name;

	PinType type;
	PinMode mode;

	Pin(int32_t aId, const std::string& aName, PinType aType, PinMode aMode)
		: id(aId), name(aName), type(aType), mode(aMode), node(nullptr)
	{}
};

struct Node
{
	ed::NodeId id;
	std::string name;
	
	std::function<void()> customContent;

	std::vector<Pin> inputs;
	std::vector<Pin> outputs;

	ImColor color;
	ImVec2 size;

	Node(int32_t aId, const std::string& aName, ImColor aColor = ImColor(1.f, 1.f, 1.f))
		: id(aId), name(aName), color(aColor)
	{ }
};

struct Link
{
	ed::LinkId id;

	ed::PinId startPin;
	ed::PinId endPin;

	ImColor color;

	Link(ed::LinkId aId, ed::PinId aStartId, ed::PinId aEndId)
		: id(aId), startPin(aStartId), endPin(aEndId)
	{ }
};