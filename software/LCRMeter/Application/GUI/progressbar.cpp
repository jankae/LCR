#include "progressbar.hpp"

ProgressBar::ProgressBar(coords_t size, color_t barColor) {
	this->size = size;
	this->barColor = barColor;
	state = 0;
}

void ProgressBar::setState(uint8_t state) {
	if(state != this->state) {
		this->state = state;
		requestRedraw();
	}
}

void ProgressBar::draw(coords_t offset) {
    /* calculate corners */
    coords_t upperLeft = offset;
    coords_t lowerRight = upperLeft;
    lowerRight.x += size.x - 1;
    lowerRight.y += size.y - 1;

    /* draw outline */
    display_SetForeground(Border);
    display_Rectangle(upperLeft.x, upperLeft.y, lowerRight.x, lowerRight.y);

    /* calculate end of bar */
    uint16_t end = util_Map(state, 0, 100, 1, size.x - 2);

    /* draw the bar */
    display_SetForeground(barColor);
    display_RectangleFull(upperLeft.x + 1, upperLeft.y + 1, upperLeft.x + end, lowerRight.y - 1);

    /* draw empty space right of the bar (in case of receding progress) */
	if (end < size.x - 2) {
		display_SetForeground(Background);
		display_RectangleFull(upperLeft.x + end + 1, upperLeft.y + 1,
				lowerRight.x - 1, lowerRight.y - 1);
	}
}
