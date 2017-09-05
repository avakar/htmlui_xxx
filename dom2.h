#include <string_view>
#include <memory>
#include <map>

// https://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core.html

namespace dom2 {

struct _node_impl;
struct _document_impl;

enum class node_type_t : uint16_t
{
	element = 1,
	text = 3,
	cdata_section = 4,
	processing_instruction = 7,
	comment = 8,
	document = 9,
	document_type = 10,
	document_fragment = 11,
};

struct _node_impl
{
	size_t ref_count = 0;

	node_type_t type;
	std::string node_name;

	_node_impl * parent;

	_node_impl * first_child = nullptr;
	_node_impl * last_child = nullptr;

	_node_impl * next_sibling = nullptr;
	_node_impl * previous_sibling = nullptr;

	_document_impl * document;
};

struct _element_impl
	: _node_impl
{
	std::string tag_name;
	std::map<std::string, std::string, std::less<>> attributes;
};

struct _character_data_impl
	: _node_impl
{
	std::string data;
};

struct _document_impl
	: _node_impl
{
	virtual _element_impl * create_element(std::string_view local_name) = 0;
};


struct document;

struct node
{
	node();
	explicit node(_node_impl * ptr);
	node(node const & o);
	node(node && o);
	node & operator=(node o);
	~node();

	node_type_t node_type() const;
	std::string_view node_name() const;

	explicit operator bool() const;

	node parent_node() const;
	node first_child() const;
	node last_child() const;
	node previous_sibling() const;
	node next_sibling() const;

	//bool has_attributes() const;
	//named_node_map attributes() const;

	document owner_document() const;

	node insert_before(node new_child, node ref_child);
	node replace_child(node new_child, node old_child);
	node remove_child(node old_child);
	node append_child(node new_child);

	bool has_child_nodes() const;

	node clone_node(bool deep) const;

	void normalize();

	std::string_view text_content() const;
	void text_content(std::string_view value);

	template <typename T>
	T cast() const;

	friend bool operator==(node const & lhs, node const & rhs);
	friend bool operator!=(node const & lhs, node const & rhs);

protected:
	_node_impl * ptr_;

	friend struct _node_access;
};

struct element final
	: node
{
	element(_element_impl * ptr)
		: node(ptr)
	{
	}

	std::string_view tag_name() const;

	bool has_attribute(std::string_view name) const;
	std::string_view get_attribute(std::string_view name) const;
	void set_attribute(std::string_view name, std::string_view value);
	void remove_attribute(std::string_view name);
};

struct character_data
	: node
{
	character_data(_character_data_impl * ptr);

	std::string_view data() const;
	void data(std::string_view value);

	size_t length() const;
};

struct text final
	: character_data
{
	text(_character_data_impl * ptr);

	std::string whole_text() const;
};


struct document final
	: node
{
	document()
	{
	}

	document(_document_impl *  ptr)
		: node(ptr)
	{
	}

	element create_element(std::string_view local_name);
};

template <typename D>
struct _property_map;

template <typename D>
struct property_map
{
	property_map()
		: ptr_(nullptr)
	{
	}

	property_map(_property_map<D> * ptr)
		: ptr_(ptr)
	{
		if (ptr)
			++ptr->ref_count;
	}

	~property_map()
	{
		if (ptr_ && --ptr_->ref_count == 0)
			delete ptr_;
	}

	typename D::element & get(element e)
	{
		return *ptr_->get_element_data(e);
	}

	typename D::text & get(text t)
	{
		return *ptr_->get_text_data(t);
	}

private:
	_property_map<D> * ptr_;
};

template <typename D>
struct _property_map
{
	virtual ~_property_map() {}

	virtual typename D::element * get_element_data(element e) = 0;
	virtual typename D::text * get_text_data(text t) = 0;

	size_t ref_count = 0;
};

struct _node_access
{
	static _node_impl * get_ptr(node const & e)
	{
		return e.ptr_;
	}
};

template <typename... Dn>
struct _element_impl_dn final
	: _element_impl
{
	std::tuple<typename Dn::element...> data;
};

template <typename... Dn>
struct _text_impl_dn final
	: _character_data_impl
{
	std::tuple<typename Dn::text...> data;
};

template <size_t I, typename... Dn>
struct _property_map_impl final
	: _property_map<std::tuple_element_t<I, std::tuple<Dn...>>>
{
	using D = std::tuple_element_t<I, std::tuple<Dn...>>;

	document doc;

	typename D::element * get_element_data(element e) override
	{
		if (e.owner_document() != doc)
			return nullptr;

		return &std::get<I>(static_cast<_element_impl_dn<Dn...> *>(_node_access::get_ptr(e))->data);
	}

	typename D::text * get_text_data(text t) override
	{
		if (t.owner_document() != doc)
			return nullptr;

		return &std::get<I>(static_cast<_text_impl_dn<Dn...> *>(_node_access::get_ptr(t))->data);
	}
};

template <typename Indexes, typename... Dn>
struct _document_factory;

template <size_t... In, typename... Dn>
struct _document_factory<std::index_sequence<In...>, Dn...>
{
	static std::tuple<document, property_map<Dn>...> create_document()
	{
		auto doc_ptr = new _document_impl();
		doc_ptr->type = node_type_t::document;
		doc_ptr->document = doc_ptr;
		doc_ptr->parent = nullptr;

		document doc(doc_ptr);

		return { std::move(doc), create_property_map<In, Dn>()... };
	}

private:
	template <size_t I, typename D>
	static property_map<D> create_property_map()
	{
		return property_map<D>(new _property_map_impl<I, Dn...>());
	}
};

template <typename... Dn>
std::tuple<document, property_map<Dn>...> create_document()
{
	return _document_factory<std::index_sequence_for<Dn...>, Dn...>::create_document();
}

}
