#include "lppch.h"
#include "LayerStack.h"

#include "Lamp/Core/Layer/Layer.h"

namespace Lamp
{
	LayerStack::LayerStack()
	{
	}

	LayerStack::~LayerStack()
	{
		Clear();
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_layers.emplace(m_layers.begin() + m_lastInsertIndex, layer);
		m_lastInsertIndex++;
		layer->OnAttach();
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_layers.emplace_back(overlay);
		overlay->OnAttach();
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_layers.begin(), m_layers.begin() + m_lastInsertIndex, layer);
		if (it != m_layers.begin() + m_lastInsertIndex)
		{
			layer->OnDetach();
			m_layers.erase(it);
			m_lastInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_layers.begin() + m_lastInsertIndex, m_layers.end(), overlay);
		if (it != m_layers.end())
		{
			overlay->OnDetach();
			m_layers.erase(it);
		}
	}

	void LayerStack::Clear()
	{
		for (Layer* layer : m_layers)
		{
			delete layer;
		}

		m_layers.clear();
	}
}