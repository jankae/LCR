#include "MenuBack.hpp"

void MenuBack::draw(coords_t offset) {
	display_SetForeground(Foreground);
	coords_t arrowStart;
	arrowStart.x = offset.x + 10;
	arrowStart.y = offset.y + size.y / 2;
	uint16_t arrowLength = size.x - 20;
	display_HorizontalLine(arrowStart.x, arrowStart.y, arrowLength);
	display_HorizontalLine(arrowStart.x, arrowStart.y + 1, arrowLength);
	display_Line(arrowStart.x, arrowStart.y, arrowStart.x + arrowLength / 3,
			arrowStart.y - arrowLength / 3);
	display_Line(arrowStart.x + 1, arrowStart.y,
			arrowStart.x + arrowLength / 3 + 1, arrowStart.y - arrowLength / 3);
	display_Line(arrowStart.x, arrowStart.y + 1, arrowStart.x + arrowLength / 3,
			arrowStart.y + arrowLength / 3 + 1);
	display_Line(arrowStart.x + 1, arrowStart.y + 1,
			arrowStart.x + arrowLength / 3 + 1,
			arrowStart.y + arrowLength / 3 + 1);
}
