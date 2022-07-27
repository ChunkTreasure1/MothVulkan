#pragma once

#include "Node.h"

class Graph
{
public:
	Graph() = default;
	~Graph() = default;

private:
	std::vector<Node> m_nodes;
	std::vector<Link> m_links;
};