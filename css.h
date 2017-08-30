#ifndef CSS_H
#define CSS_H

#include <variant>
#include <string>
#include <stdint.h>
#include <tuple>

namespace css {

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

enum class font_style
{
	kw_normal,
	kw_italic,
	kw_oblique,
};

struct css_percentage
{
	int value;
};

struct length
{
	int value;
};

struct color
{
	uint8_t r, g, b, a;

	bool operator<(color const & rhs) const
	{
		return std::tie(r, g, b, a) < std::tie(rhs.r, rhs.g, rhs.b, rhs.a);
	}
};

using css_dimension = std::variant<css_auto, css_percentage, length>;

struct style
{
	color background_color;

	css_display display;

	css_dimension width;
	css_dimension height;

	css_dimension margin_left;
	css_dimension margin_right;
	css_dimension margin_top;
	css_dimension margin_bottom;

	font_style font_style = font_style::kw_normal;
	css_dimension font_size;
	std::string font_family;
};

}

#endif
