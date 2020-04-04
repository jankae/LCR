#include "entry.hpp"

// Specialized string creation for use with float values (no unit symbol supported so far)
template <>
void Entry<float>::CreateString()
{
    Unit::SIStringFromFloat(inputString, length, *value);
}
