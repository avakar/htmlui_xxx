#include "dom.h"

void dom::node::remove()
{
	if (prev_)
		prev_->next_ = next_;
	if (next_)
		next_->prev_ = prev_;

	next_ = nullptr;

	node * cur = this;
	while (cur)
	{
		if (cur->type_ == node_type::element)
		{
			element * e = static_cast<element *>(cur);

			if (e->first_child_)
			{
				e->last_child_->next_ = cur->next_;
				cur = e->first_child_;
			}
			else
			{
				cur = e->next_;
			}

			delete e;
		}
		else if (cur->type_ == node_type::text)
		{
			text_node * n = static_cast<text_node *>(cur);
			cur = n->next_;
			delete n;
		}
	}
}
