#include "StepView.h"
#include "../../../ComponentGenerator.h"

ui::Page StepView::GenerateStepViewPage() {
	ui::Page page;
	page.set_page_id("debate");
	page.set_title("Debate Step View");

	ui::Component container = ComponentGenerator::createContainer(
		"stepViewMain",
		"min-h-screen flex items-center justify-center",
		"bg-gray-900",
		"p-6",
		"",
		"",
		"",
		""
	);

	ui::Component testButton = ComponentGenerator::createButton(
		"stepViewTestButton",
		"test",
		"",
		"bg-blue-600",
		"hover:bg-blue-700",
		"text-white",
		"px-6 py-3",
		"rounded",
		"text-sm"
	);

	ComponentGenerator::addChild(&container, testButton);

	ui::Component* root = page.add_components();
	root->CopyFrom(container);
	return page;
}
