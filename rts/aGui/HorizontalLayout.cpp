/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include "HorizontalLayout.h"

#include "Gui.h"
#include "Rendering/GL/myGL.h"

namespace agui
{

HorizontalLayout::HorizontalLayout(GuiElement* parent) : GuiElement(parent)
{
}

void HorizontalLayout::DrawSelf()
{
	if (!visibleBorder)
		return;

	gui->SetColor(1.0f, 1.0f, 1.0f, Opacity());
	DrawOutline();
}

void HorizontalLayout::GeometryChangeSelf()
{
	GuiElement::GeometryChangeSelf();

	if (children.empty())
		return;

	unsigned numFixedObjs = 0;
	unsigned sumObjWeight = 0;

	float totalFixedSize = 0.0;

	for (const auto& child: children) {
		if (!child->SizeFixed())
			continue;

		numFixedObjs += 1;
		totalFixedSize += child->GetSize()[0];
	}

	for (const auto& child: children) {
		sumObjWeight += child->Weight();
	}

	const float hspaceBorder = 2.0f * borderSpacing;
	const float hspaceTotal  = std::max(1.0f, float(sumObjWeight - numFixedObjs));
	const float hspacePerObj = (size[0] - float(sumObjWeight - 1) * itemSpacing - hspaceBorder - totalFixedSize) / hspaceTotal;

	float startX = pos[0] + borderSpacing;

	for (const auto& child: children) {
		child->SetPos(startX, pos[1] + borderSpacing);

		if (child->SizeFixed()) {
			child->SetSize(child->GetSize()[0], size[1] - hspaceBorder, true);
			startX += child->GetSize()[0] + itemSpacing;
		} else {
			child->SetSize(hspacePerObj * float(child->Weight()), size[1] - hspaceBorder);
			startX += hspacePerObj*float(child->Weight()) + itemSpacing;
		}
	}
}

}
