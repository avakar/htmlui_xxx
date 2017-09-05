#include "dom3.h"
using namespace dom3;

node_intf::node_intf(uint16_t node_type, document_intf * document)
	: type_(node_type), document_(document)
{
}

node_intf::~node_intf()
{
	while (first_child_)
	{
		if (first_child_->ref_count_ > 1 || !first_child_->first_child_)
		{
			first_child_->parent_ = nullptr;
			first_child_->next_sibling_->previous_sibling_ = nullptr;
			first_child_ = std::move(first_child_->next_sibling_);
		}
		else
		{
			first_child_->next_sibling_->previous_sibling_ = 
			first_child_->last_child_->next_sibling_ = std::move(first_child_->next_sibling_);
		}
	}
}

void node_intf::addref()
{
	++ref_count_;

	if (document_)
		document_->addref();
}

void node_intf::release()
{
	if (document_)
		document_->release();

	if (--ref_count_ == 0)
		delete this;
}

node node_intf::parent_node() const
{
	return node(parent_);
}

document node_intf::owner_document() const
{
	return document(document_);
}

node node_intf::remove_child(node old_child)
{
	assert(old_child->parent_ == this);

	if (old_child->previous_sibling_)
		old_child->previous_sibling_->next_sibling_ = std::move(old_child->next_sibling_);
	else
		first_child_ = std::move(old_child->next_sibling_);

	if (old_child->next_sibling_)
		old_child->next_sibling_->previous_sibling_ = old_child->previous_sibling_;
	else
		last_child_ = old_child->previous_sibling_;

	old_child->next_sibling_.reset();
	old_child->previous_sibling_ = nullptr;
	old_child->parent_ = nullptr;

	return old_child;
}

node node_intf::append_child(node new_child)
{
	if (node_intf * p = new_child->parent_)
		p->remove_child(new_child);

	if (!last_child_)
		first_child_ = new_child;
	else
		last_child_->next_sibling_ = new_child;

	new_child->previous_sibling_ = last_child_;
	last_child_ = new_child.get();
	new_child->parent_ = this;

	return new_child;
}

document_intf::document_intf()
	: node_intf(node_intf::document_node, nullptr)
{
}

element_intf::element_intf(std::string_view local_name, document_intf * doc)
	: node_intf(node_intf::element_node, doc), tag_name_(local_name)
{

}

character_data_intf::character_data_intf(uint16_t node_type, document_intf * doc)
	: node_intf(node_type, doc)
{
}

std::string_view character_data_intf::data() const
{
	return data_;
}

void character_data_intf::data(std::string_view value)
{
	data_ = value;
}

size_t character_data_intf::length() const
{
	return data_.size();
}

text_intf::text_intf(document_intf * doc)
	: character_data_intf(node_intf::text_node, doc)
{
}
