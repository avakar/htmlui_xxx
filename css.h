#ifndef CSS_H
#define CSS_H

#include <variant>
struct css_auto
{
};

enum class css_display
{
	kw_none,
	kw_inline,
	kw_block,
	kw_inline_block,
};

struct css_percentage
{
	float value;
};

struct css_length
{
	float value;
};

struct css_color
{
	uint8_t r, g, b;
};

using css_dimension = std::variant<css_auto, css_percentage, css_length>;

#endif
