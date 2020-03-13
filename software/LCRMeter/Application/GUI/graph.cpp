#include "graph.hpp"

Graph::Graph(const int32_t *values, uint16_t num, uint16_t height,
		color_t color, const Unit::unit *unit[]) {
	this->values = values;
	this->color = color;
	size.x = num + 2;
	size.y = height;
	this->unit = unit;
}

void Graph::newColor(color_t color) {
	this->color = color;
	requestRedraw();
}

void Graph::newData(const int32_t *data) {
	values = data;
	requestRedrawFull();
}

void Graph::draw(coords_t offset) {
    /* calculate corners */
    coords_t upperLeft = offset;
	coords_t lowerRight = upperLeft;
	lowerRight.x += size.x - 1;
	lowerRight.y += size.y - 1;

	/* draw rectangle */
	display_SetForeground(Border);
	display_Rectangle(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);

	/* find min/max value */
	int32_t min = INT32_MAX;
	int32_t max = INT32_MIN;
	uint16_t num = size.x - 2;
	uint16_t i;
	for (i = 0; i < num; i++) {
		if (values[i] < min)
			min = values[i];
		if (values[i] > max)
			max = values[i];
	}

	/* draw graph data */
	display_SetForeground(color);
	int32_t lastY = util_Map(values[0], min, max, lowerRight.y - 1,
			upperLeft.y + 1);
	for (i = 1; i < num; i++) {
		int32_t Y = util_Map(values[i], min, max, lowerRight.y - 1,
				upperLeft.y + 1);
		display_Line(upperLeft.x + i, lastY, upperLeft.x + 1 + i, Y);
		lastY = Y;
	}

	/* display min/max data */
	display_SetFont(Font_Medium);
	char buf[11];
	Unit::StringFromValue(buf, 7, max, unit);
	display_String(lowerRight.x - 41, upperLeft.y + 2, buf);
	Unit::StringFromValue(buf, 7, min, unit);
	display_String(lowerRight.x - 41, lowerRight.y - 8, buf);
}
