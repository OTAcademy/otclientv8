#include "uigraph.h"
#include <framework/graphics/drawqueue.h>
#include <framework/platform/platformwindow.h>

UIGraph::UIGraph()
{
	m_needsUpdate = false;
	m_showLabes = true;
	m_showInfo = false;

	m_capacity = 100;
	m_ignores = 0;
}

void UIGraph::drawSelf(Fw::DrawPane drawPane)
{
	if (drawPane != Fw::ForegroundPane)
		return;

	if (m_backgroundColor.aF() > Fw::MIN_ALPHA) {
		Rect backgroundDestRect = m_rect;
		backgroundDestRect.expand(-m_borderWidth.top, -m_borderWidth.right, -m_borderWidth.bottom, -m_borderWidth.left);
		drawBackground(m_rect);
	}

	drawImage(m_rect);

	if (m_needsUpdate) {
		cacheGraphs();
	}

	if (!m_graphs.empty()) {
		Rect dest = getPaddingRect();

		// draw graph first
		for (auto& graph : m_graphs) {
			if (!graph.visible) continue;

			g_drawQueue->addLine(graph.points, graph.width, graph.lineColor);
		}

		// then update if needed and draw vertical line if hovered
		bool updated = false;
		for (auto& graph : m_graphs) {
			if (!graph.visible) continue;

			if (m_showInfo && isHovered()) {
				updateGraph(graph, updated);
				g_drawQueue->addLine({ graph.infoLine[0], graph.infoLine[1] }, 1, graph.infoLineColor);
			}
		}

		// reposition intersecting rects and keep them within rect bounds
		if (updated) {
			updateInfoBoxes();
		}

		// now we draw indicators on the graph lines
		for (auto& graph : m_graphs) {
			if (!graph.visible) continue;

			if (m_showInfo && isHovered()) {
				g_drawQueue->addFilledRect(graph.infoIndicatorBg, Color::white);
				g_drawQueue->addFilledRect(graph.infoIndicator, graph.lineColor);
			}
		}

		// lastly we can draw info boxes with value
		for (auto& graph : m_graphs) {
			if (!graph.visible) continue;

			if (m_showInfo && isHovered()) {
				g_drawQueue->addFilledRect(graph.infoRectBg, graph.infoTextBg);
				g_drawQueue->addFilledRect(graph.infoRectIcon, graph.lineColor);
				m_font->drawText(graph.infoValue, graph.infoRect, Fw::AlignLeftCenter, Color::black, m_shadow);
			}
		}

		if (!m_title.empty())
			g_drawQueue->addText(m_font, m_title, dest, Fw::AlignTopCenter);
		if (m_showLabes) {
			g_drawQueue->addText(m_font, m_lastValue, dest, Fw::AlignTopRight);
			g_drawQueue->addText(m_font, m_maxValue, dest, Fw::AlignTopLeft);
			g_drawQueue->addText(m_font, m_minValue, dest, Fw::AlignBottomLeft);
		}
	}

	drawBorder(m_rect);
	drawIcon(m_rect);
	drawText(m_rect);
}

void UIGraph::clear()
{
	m_graphs.clear();
}

void UIGraph::setLineWidth(size_t index, int width) {
	if (m_graphs.size() <= index - 1) {
		g_logger.warning(stdext::format("[UIGraph::setLineWidth (%s)] Graph of index %d out of bounds.", getId(), index));
		return;
	}

	auto& graph = m_graphs[index - 1];
	graph.width = width;
	m_needsUpdate = true;
}

size_t UIGraph::createGraph()
{
	auto graph = GraphData();

	graph.points = {};
	graph.values = {};

	graph.infoLine[0] = Point();
	graph.infoLine[1] = Point();

	graph.originalInfoRect = Rect();
	graph.infoRect = Rect();
	graph.infoRectBg = Rect();
	graph.infoRectIcon = Rect(0, 0, 10, 10);
	graph.infoIndicator = Rect(0, 0, 5, 5);
	graph.infoIndicatorBg = Rect(0, 0, 7, 7);

	graph.infoText = "Value: ";

	graph.infoLineColor = Color::white;
	graph.infoTextBg = Color(0, 0, 0, 100);
	graph.lineColor = Color::white;

	graph.width = 1;
	graph.infoIndex = -1;

	graph.visible = true;

	m_graphs.push_back(graph);
	return m_graphs.size();
}

void UIGraph::addValue(size_t index, int value, bool ignoreSmallValues)
{
	if (m_graphs.size() <= index - 1) {
		g_logger.warning(stdext::format("[UIGraph::addValue (%s)] Graph of index %d out of bounds.", getId(), index));
		return;
	}

	auto& graph = m_graphs[index - 1];

	if (ignoreSmallValues) {
		if (!graph.values.empty() && graph.values.back() <= 2 && value <= 2 && ++m_ignores <= 10)
			return;
		m_ignores = 0;
	}

	graph.values.push_back(value);
	while (graph.values.size() > m_capacity)
		graph.values.pop_front();

	m_needsUpdate = true;
}

void UIGraph::setLineColor(size_t index, const Color& color)
{
	if (m_graphs.size() <= index - 1) {
		g_logger.warning(stdext::format("[UIGraph::setLineColor (%s)] Graph of index %d out of bounds.", getId(), index));
		return;
	}

	auto& graph = m_graphs[index - 1];
	graph.lineColor = color;
}

void UIGraph::setInfoText(size_t index, const std::string& text)
{
	if (m_graphs.size() <= index - 1) {
		g_logger.warning(stdext::format("[UIGraph::setInfoText (%s)] Graph of index %d out of bounds.", getId(), index));
		return;
	}

	auto& graph = m_graphs[index - 1];
	graph.infoText = text;
}

void UIGraph::setGraphVisible(size_t index, bool visible)
{
	if (m_graphs.size() <= index - 1) {
		g_logger.warning(stdext::format("[UIGraph::setGraphVisible (%s)] Graph of index %d out of bounds.", getId(), index));
		return;
	}

	auto& graph = m_graphs[index - 1];
	graph.visible = visible;
}

void UIGraph::setInfoLineColor(size_t index, const Color& color)
{
	if (m_graphs.size() <= index - 1) {
		g_logger.warning(stdext::format("[UIGraph::setInfoLineColor (%s)] Graph of index %d out of bounds.", getId(), index));
		return;
	}

	auto& graph = m_graphs[index - 1];
	graph.infoLineColor = color;
}

void UIGraph::setTextBackground(size_t index, const Color& color)
{
	if (m_graphs.size() <= index - 1) {
		g_logger.warning(stdext::format("[UIGraph::setTextBackground (%s)] Graph of index %d out of bounds.", getId(), index));
		return;
	}

	auto& graph = m_graphs[index - 1];
	graph.infoTextBg = color;
}

void UIGraph::cacheGraphs()
{
	if (!m_needsUpdate)
		return;

	if (!m_rect.isEmpty() && m_rect.isValid()) {
		if (!m_graphs.empty()) {
			Rect rect = getPaddingRect();

			float paddingX = static_cast<float>(rect.x());
			float paddingY = static_cast<float>(rect.y());
			float graphWidth = static_cast<float>(rect.width());
			float graphHeight = static_cast<float>(rect.height());

			float minValue = 0.0f;
			float maxValue = 0.0f;
			for (auto& graph : m_graphs) {
				if (graph.values.empty())
					continue;

				graph.points.clear();

				auto [minValueIter, maxValueIter] = std::minmax_element(graph.values.begin(), graph.values.end());
				minValue = *minValueIter;
				maxValue = *maxValueIter;
				float range = maxValue - minValue;
				if (range == 0.0f)
					range = 1.0f;

				float pointSpacing = graphWidth / std::max(static_cast<int>(graph.values.size()) - 1, 1);
				for (size_t i = 0; i < graph.values.size(); ++i) {
					float x = paddingX + i * pointSpacing;
					float y = paddingY + graphHeight - ((graph.values[i] - minValue) / range) * graphHeight;
					graph.points.push_back({ static_cast<int>(x), static_cast<int>(y) });
				}
			}

			m_minValue = std::to_string(static_cast<int>(minValue));
			m_maxValue = std::to_string(static_cast<int>(maxValue));
			if (!m_graphs[0].values.empty())
				m_lastValue = std::to_string(m_graphs[0].values.back());
			else
				m_lastValue = "0";
		}

		m_needsUpdate = false;
	}
}

void UIGraph::updateGraph(GraphData& graph, bool& updated)
{
	if (graph.values.empty())
		return;

	auto dest = getPaddingRect();
	auto mousePos = g_window.getMousePosition();
	float graphWidth = static_cast<float>(dest.width());
	float graphHeight = static_cast<float>(dest.height());
	float pointSpacing = graphWidth / std::max(static_cast<int>(graph.values.size()) - 1, 1);

	int dataIndex = static_cast<int>((mousePos.x - dest.left()) / pointSpacing + 0.5f);
	dataIndex = std::clamp(dataIndex, 0, static_cast<int>(graph.values.size()) - 1);

	if (graph.infoIndex != dataIndex) {
		graph.infoIndex = dataIndex;

		float snappedX = dest.left() + dataIndex * pointSpacing;
		int value = graph.values[graph.infoIndex];

		graph.infoLine[0] = Point(snappedX, dest.top());
		graph.infoLine[1] = Point(snappedX, dest.bottom());

		graph.infoValue = stdext::format("%s %d", graph.infoText, value);

		auto [minValueIter, maxValueIter] = std::minmax_element(graph.values.begin(), graph.values.end());
		float minValue = static_cast<float>(*minValueIter);
		float maxValue = static_cast<float>(*maxValueIter);
		float range = maxValue - minValue;
		if (range == 0.0f)
			range = 1.0f;

		float pointY = dest.top() + graphHeight - ((value - minValue) / range) * graphHeight;

		auto textSize = m_font->calculateTextRectSize(graph.infoValue);
		graph.infoRectBg.setWidth(textSize.width() + 16);
		graph.infoRectBg.setHeight(textSize.height());
		graph.infoRectBg.expand(4);
		graph.infoRectBg.moveTop(pointY - graph.infoRectBg.height() / 2.0);
		if (snappedX >= dest.horizontalCenter())
			graph.infoRectBg.moveRight(snappedX - 10);
		else
			graph.infoRectBg.moveLeft(snappedX + 10);

		graph.infoRect.setWidth(textSize.width());
		graph.infoRect.setHeight(textSize.height());
		graph.infoRect.moveRight(graph.infoRectBg.right() - 4);
		graph.infoRect.moveVerticalCenter(graph.infoRectBg.verticalCenter());

		int iconPadding = graph.infoRectBg.height() - graph.infoRectIcon.width();
		graph.infoRectIcon.moveLeft(graph.infoRectBg.left() + (iconPadding / 2.0));
		graph.infoRectIcon.moveVerticalCenter(graph.infoRectBg.verticalCenter());

		graph.infoIndicator.moveLeft(snappedX - 3);
		graph.infoIndicator.moveTop(pointY - 3);
		graph.infoIndicatorBg.moveCenter(graph.infoIndicator.center());

		graph.originalInfoRect = graph.infoRectBg;
		updated = true;
	}
}

void UIGraph::updateInfoBoxes()
{
	auto dest = getPaddingRect();
	std::vector<Rect> occupiedSpaces(m_graphs.size());
	for (size_t i = 0; i < m_graphs.size(); ++i) {
		auto& graph = m_graphs[i];

		graph.infoRectBg = graph.originalInfoRect;
		graph.infoRect.moveVerticalCenter(graph.infoRectBg.verticalCenter());
		graph.infoRectIcon.moveVerticalCenter(graph.infoRectBg.verticalCenter());

		occupiedSpaces[i] = graph.infoRectBg;

		for (size_t j = 0; j < occupiedSpaces.size(); ++j) {
			if (i == j) continue; // graph's space, ignore

			auto& space = occupiedSpaces[j];
			// first check if this graph occupies another graph's space and move it above the space
			if (space.intersects(graph.infoRectBg)) {
				graph.infoRectBg.moveBottom(space.top() - 2);
				graph.infoRect.moveVerticalCenter(graph.infoRectBg.verticalCenter());
				graph.infoRectIcon.moveVerticalCenter(graph.infoRectBg.verticalCenter());
			}

			// lets make sure its within bounds of this widget
			if (graph.infoRectBg.top() < dest.top()) {
				graph.infoRectBg.moveTop(dest.top());
				graph.infoRect.moveVerticalCenter(graph.infoRectBg.verticalCenter());
				graph.infoRectIcon.moveVerticalCenter(graph.infoRectBg.verticalCenter());
			}

			// if we just moved due to bounds check, we have to make sure we are not occuping another graph
			// this time move it below the occupied space
			if (space.intersects(graph.infoRectBg)) {
				graph.infoRectBg.moveTop(space.bottom() + 2);
				graph.infoRect.moveVerticalCenter(graph.infoRectBg.verticalCenter());
				graph.infoRectIcon.moveVerticalCenter(graph.infoRectBg.verticalCenter());
			}

			// and check again if we are within bounds
			if (graph.infoRectBg.bottom() > dest.bottom()) {
				graph.infoRectBg.moveBottom(dest.bottom());
				graph.infoRect.moveVerticalCenter(graph.infoRectBg.verticalCenter());
				graph.infoRectIcon.moveVerticalCenter(graph.infoRectBg.verticalCenter());
			}
		}

		occupiedSpaces[i] = graph.infoRectBg;
	}
}

void UIGraph::onStyleApply(const std::string& styleName, const OTMLNodePtr& styleNode)
{
	UIWidget::onStyleApply(styleName, styleNode);

	for (const OTMLNodePtr& node : styleNode->children()) {
		if (node->tag() == "capacity")
			setCapacity(node->value<int>());
		else if (node->tag() == "title")
			setTitle(node->value());
		else if (node->tag() == "show-labels")
			setShowLabels(node->value<bool>());
		else if (node->tag() == "show-info") // draw info (vertical line, labels with values) on mouse position
			setShowInfo(node->value<bool>());
	}
}

void UIGraph::onGeometryChange(const Rect& oldRect, const Rect& newRect)
{
	UIWidget::onGeometryChange(oldRect, newRect);
	m_needsUpdate = true;
}

void UIGraph::onLayoutUpdate()
{
	UIWidget::onLayoutUpdate();
	m_needsUpdate = true;
}

void UIGraph::onVisibilityChange(bool visible)
{
	UIWidget::onVisibilityChange(visible);
	m_needsUpdate = visible;
}
