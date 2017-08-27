#include "css.h"
#include <string>
#include <memory>

struct fmt_context;

namespace dom {

struct string_list
{
	std::string item(size_t index);
	size_t length();
	bool contains(std::string_view str);

	size_t size();
	std::string operator[](size_t index);
};



enum class node_type
{
	element,
	text,
};

struct element;

struct node
{
	node_type type_;

	element * parent_;

	node * next_ = nullptr;
	node * prev_ = nullptr;

	int x, y, dx, dy;

	void remove();
};

struct text final
	: node
{
	std::string value;
	std::wstring render_value;
};

struct element final
	: node
{
	std::string name;

	node * first_child_ = nullptr;
	node * last_child_ = nullptr;

	text * append_text(std::string str)
	{
		text * n = new text();
		n->type_ = node_type::text;
		n->parent_ = this;
		n->value = std::move(str);

		n->prev_ = last_child_;
		if (last_child_)
			last_child_->next_ = n;
		last_child_ = n;
		if (!first_child_)
			first_child_ = n;

		return n;
	}

	element * append_element(std::string name)
	{
		element * n = new element();
		n->type_ = node_type::element;
		n->parent_ = this;
		n->name = std::move(name);

		n->prev_ = last_child_;
		if (last_child_)
			last_child_->next_ = n;
		last_child_ = n;
		if (!first_child_)
			first_child_ = n;

		return n;
	}

	css_color background_color;

	css_display display;

	css_dimension width;
	css_dimension height;

	css_dimension margin_left;
	css_dimension margin_right;
	css_dimension margin_top;
	css_dimension margin_bottom;

	css_font_style font_style = css_font_style::kw_normal;
	css_dimension font_size;
	std::string font_family;


	std::shared_ptr<fmt_context> fmt_context;

	bool is_inline_level() const
	{
		return display == css_display::kw_inline || display == css_display::kw_inline_block;
	}

	bool is_block_level() const
	{
		return display == css_display::kw_block;
	}
};

}
