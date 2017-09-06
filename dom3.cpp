#include "dom3.h"
using namespace dom3;

node_intf::node_intf(uint16_t node_type, document_intf * document)
	: type_(node_type), document_(document)
{
}

node_intf::~node_intf()
{
	// XXX
}

void node_intf::addref()
{
	node_intf * cur = this;

	while (cur && ++cur->ref_count_ == 1)
		cur = cur->parent_? cur->parent_: cur->document_;
}

void node_intf::delete_tree(node_intf * n)
{
	while (n)
	{
		node_intf * next;

		assert(n->ref_count_ == 0);
		if (n->first_child_)
		{
			next = n->first_child_;
			n->last_child_->next_sibling_ = n->next_sibling_;
		}
		else
		{
			next = n->next_sibling_;
		}

		delete n;
		n = next;
	}
}

void node_intf::release()
{
	node_intf * cur = this;

	while (--cur->ref_count_ == 0)
	{
		node_intf * next = cur->parent_? cur->parent_: cur->document_;

		if (!cur->parent_)
			delete_tree(cur);

		cur = next;
	}
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
	assert(old_child);
	assert(old_child->parent_ == this);

	if (old_child->previous_sibling_)
		old_child->previous_sibling_->next_sibling_ = old_child->next_sibling_;
	else
		first_child_ = old_child->next_sibling_;

	if (old_child->next_sibling_)
		old_child->next_sibling_->previous_sibling_ = old_child->previous_sibling_;
	else
		last_child_ = old_child->previous_sibling_;

	old_child->next_sibling_ = nullptr;
	old_child->previous_sibling_ = nullptr;
	old_child->parent_ = nullptr;

	assert(old_child->document_ == document_ || (document_ == nullptr && old_child->document_ == this));
	assert(old_child->ref_count_ >= 1);
	old_child->document_->addref();
	this->release();

	return old_child;
}

node node_intf::append_child(node new_child)
{
	assert(new_child);
	assert(new_child->document_ == document_ || (document_ == nullptr && new_child->document_ == this));

	assert(new_child->ref_count_ >= 1);
	this->addref();

	if (node_intf * p = new_child->parent_)
		p->remove_child(new_child);

	if (!last_child_)
		first_child_ = new_child.get();
	else
		last_child_->next_sibling_ = new_child.get();

	new_child->previous_sibling_ = last_child_;
	last_child_ = new_child.get();
	new_child->parent_ = this;

	new_child->document_->release();
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
