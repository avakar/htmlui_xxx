#include "css.h"
#include <string>

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

	float x, y, dx, dy;

	void remove();
};

struct text_node final
	: node
{
	std::string value;
};

struct element final
	: node
{
	node * first_child_ = nullptr;
	node * last_child_ = nullptr;

	css_color background_color;

	css_display display;

	css_dimension width;
	css_dimension height;

	css_dimension margin_left;
	css_dimension margin_right;
	css_dimension margin_top;
	css_dimension margin_bottom;

	bool is_inline_level() const
	{
		return display == css_display::kw_inline || display == css_display::kw_inline_block;
	}

	bool is_block_level() const
	{
		return display == css_display::kw_block;
	}
};

struct document final
{
	~document()
	{
		this->clear();
	}

	void clear()
	{
		if (root_)
		{
			root_->remove();
			root_ = nullptr;
		}
	}

	element * create_root()
	{
		element * e = new element();
		this->clear();
		root_ = e;
		return e;
	}

	element * root() { return root_; }

private:
	element * root_ = nullptr;
};

}
