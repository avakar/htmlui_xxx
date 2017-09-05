#include "dom2.h"
#include <assert.h>
using namespace dom2;

node::node()
	: ptr_(nullptr)
{
}

node::node(_node_impl * ptr)
	: ptr_(ptr)
{
	if (ptr_)
		++ptr_->ref_count;
}

node::node(node const & o)
	: ptr_(o.ptr_)
{
	if (ptr_)
		++ptr_->ref_count;
}

node::node(node && o)
	: ptr_(o.ptr_)
{
	o.ptr_ = nullptr;
}

node & node::operator=(node o)
{
	std::swap(ptr_, o.ptr_);
	return *this;
}

node::~node()
{
	if (ptr_)
		--ptr_->ref_count;
}

node_type_t node::node_type() const
{
	return ptr_->type;
}

std::string_view node::node_name() const
{
	return ptr_->node_name;
}

node::operator bool() const
{
	return ptr_ != nullptr;
}

node node::parent_node() const
{
	return node(ptr_? ptr_->parent: nullptr);
}

node node::first_child() const
{
	return node(ptr_? ptr_->first_child: nullptr);
}

node node::last_child() const
{
	return node(ptr_? ptr_->last_child: nullptr);
}

node node::previous_sibling() const
{
	return node(ptr_? ptr_->previous_sibling: nullptr);
}

node node::next_sibling() const
{
	return node(ptr_? ptr_->next_sibling: nullptr);
}

document node::owner_document() const
{
	return document(ptr_? ptr_->document: nullptr);
}

node node::insert_before(node new_child, node ref_child)
{
	assert(ptr_);
	assert(new_child);

	assert(!ref_child || ref_child.parent_node() == *this);

	if (new_child.parent_node())
		new_child.parent_node().remove_child(new_child);

	if (!ref_child)
	{
		if (ptr_->last_child)
		{
			ptr_->last_child->next_sibling = new_child.ptr_;
			new_child.ptr_->previous_sibling = ptr_->last_child;
		}
		else
		{
			ptr_->first_child = new_child.ptr_;
		}

		ptr_->last_child = new_child.ptr_;
	}
	else
	{
		new_child.ptr_->previous_sibling = ref_child.ptr_->previous_sibling;
		new_child.ptr_->next_sibling = ref_child.ptr_;

		if (new_child.ptr_->previous_sibling)
			new_child.ptr_->previous_sibling->next_sibling = new_child.ptr_;
		else
			ptr_->first_child = new_child.ptr_;

		ref_child.ptr_->previous_sibling = new_child.ptr_;
	}

	new_child.ptr_->parent = ptr_;
	++new_child.ptr_->ref_count;

	return new_child;
}

node node::remove_child(node old_child)
{
	assert(ptr_);
	assert(old_child);
	assert(old_child.ptr_->parent == ptr_);

	if (old_child.ptr_->previous_sibling)
		old_child.ptr_->previous_sibling->next_sibling = old_child.ptr_->next_sibling;
	else
		ptr_->first_child = old_child.ptr_->next_sibling;

	if (old_child.ptr_->next_sibling)
		old_child.ptr_->next_sibling->previous_sibling = old_child.ptr_->previous_sibling;
	else
		ptr_->last_child = old_child.ptr_->previous_sibling;

	old_child.ptr_->previous_sibling = nullptr;
	old_child.ptr_->next_sibling = nullptr;
	old_child.ptr_->parent = nullptr;

	assert(old_child.ptr_->ref_count >= 2);
	--old_child.ptr_->ref_count;

	return old_child;
}

node node::append_child(node new_child)
{
	assert(ptr_);
	assert(new_child);

	if (new_child.parent_node())
		new_child.parent_node().remove_child(new_child);

	if (!ptr_->last_child)
		ptr_->first_child = new_child.ptr_;
	else
		new_child.ptr_->previous_sibling = ptr_->last_child;

	ptr_->last_child = new_child.ptr_;

	new_child.ptr_->parent = ptr_;
	++new_child.ptr_->ref_count;

	return new_child;
}

bool node::has_child_nodes() const
{
	return ptr_ && ptr_->first_child != nullptr;
}

template <>
text node::cast() const
{
	return ptr_->type == node_type_t::text? text(static_cast<_character_data_impl *>(ptr_)): nullptr;
}

template <>
element node::cast() const
{
	return ptr_->type == node_type_t::element? element(static_cast<_element_impl *>(ptr_)): nullptr;
}

template <>
document node::cast() const
{
	return ptr_->type == node_type_t::document? document(static_cast<_document_impl *>(ptr_)) : nullptr;
}

bool dom2::operator==(node const & lhs, node const & rhs)
{
	return lhs.ptr_ == rhs.ptr_;
}

bool dom2::operator!=(node const & lhs, node const & rhs)
{
	return lhs.ptr_ != rhs.ptr_;
}

std::string_view element::tag_name() const
{
	return static_cast<_element_impl const *>(ptr_)->tag_name;
}

bool element::has_attribute(std::string_view name) const
{
	auto * ptr = static_cast<_element_impl const *>(ptr_);
	return ptr->attributes.find(name) != ptr->attributes.end();
}

std::string_view element::get_attribute(std::string_view name) const
{
	auto * ptr = static_cast<_element_impl const *>(ptr_);
	auto it = ptr->attributes.find(name);
	return it != ptr->attributes.end()? it->second: "";
}

void element::set_attribute(std::string_view name, std::string_view value)
{
	auto * ptr = static_cast<_element_impl *>(ptr_);
	auto it = ptr->attributes.find(name);
	if (it != ptr->attributes.end())
		it->second = value;
	else
		ptr->attributes.emplace(name, value);
}

void element::remove_attribute(std::string_view name)
{
	auto * ptr = static_cast<_element_impl *>(ptr_);
	auto it = ptr->attributes.find(name);
	ptr->attributes.erase(it);
}

character_data::character_data(_character_data_impl * ptr)
	: node(ptr)
{
}

std::string_view character_data::data() const
{
	auto * ptr = static_cast<_character_data_impl const *>(ptr_);
	return ptr->data;
}

void character_data::data(std::string_view value)
{
	auto * ptr = static_cast<_character_data_impl *>(ptr_);
	ptr->data = value;
}

size_t character_data::length() const
{
	auto * ptr = static_cast<_character_data_impl const *>(ptr_);
	return ptr->data.size();
}

text::text(_character_data_impl * ptr)
	: character_data(ptr)
{
}

std::string text::whole_text() const
{
	auto * ptr = static_cast<_character_data_impl const *>(ptr_);
	size_t total_len = ptr->data.size();

	node cur = this->next_sibling();
	while (cur)
	{
		if (text t = cur.cast<text>())
			total_len += t.length();
		cur = cur.next_sibling();
	}

	node first = cur;
	cur = this->previous_sibling();
	while (cur)
	{
		first = cur;
		if (text t = cur.cast<text>())
			total_len += t.length();
		cur = cur.previous_sibling();
	}

	std::string r;
	r.reserve(total_len);
	while (first)
	{
		if (text t = cur.cast<text>())
			r.append(t.data());
		first = first.next_sibling();
	}

	return r;
}

element document::create_element(std::string_view local_name)
{
}
