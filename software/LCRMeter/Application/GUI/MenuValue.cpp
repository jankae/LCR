#include "MenuValue.hpp"

template <> void MenuValue<float>::CreateUnitString(char *s, uint8_t len) {
	Unit::SIStringFromFloat(s, len, *value);
}
