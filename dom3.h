#include <string_view>
#include <memory>
#include <map>
#include <type_traits>

// https://dom.spec.whatwg.org/

namespace dom3 {

template <typename T, typename... Tn>
struct _is_one_of
	: std::false_type
{
};

template <typename T, typename... Tn>
struct _is_one_of<T, T, Tn...>
	: std::true_type
{
};

template <typename T, typename T0, typename... Tn>
struct _is_one_of<T, T0, Tn...>
	: _is_one_of<T, Tn...>
{
};

struct node_intf;
struct element_intf;
struct text_intf;
struct document_intf;

template <typename T>
struct is_node_intf
	: _is_one_of<T, element_intf, text_intf, document_intf>
{
};

template <typename T>
struct node_ptr final
{
	static_assert(std::is_base_of_v<node_intf, T>);

	node_ptr();

	explicit node_ptr(T * ptr);

	template <typename U>
	node_ptr(node_ptr<U> const & o);

	template <typename U>
	node_ptr(node_ptr<U> && o);

	node_ptr(node_ptr const & o);
	node_ptr(node_ptr && o);
	node_ptr & operator=(node_ptr o);
	~node_ptr();

	T * release();
	void reset(T * node = nullptr);

	explicit operator bool() const;
	T * get() const;

	T & operator*() const;
	T * operator->() const;

	template <typename U>
	std::enable_if_t<is_node_intf<U>::value, node_ptr<U>> cast();

	bool operator==(node_ptr const & rhs) const;
	bool operator!=(node_ptr const & rhs) const;

private:
	T * ptr_;
};

using node = node_ptr<node_intf>;
using element = node_ptr<element_intf>;
using text = node_ptr<text_intf>;
using document = node_ptr<document_intf>;

struct node_intf
{
	enum : uint16_t
	{
		element_node = 1,
		text_node = 3,
		cdata_section_node = 4,
		processing_instruction_node = 7,
		comment_node = 8,
		document_node = 9,
		document_type_node = 10,
		document_fragment_node = 11,
	};

	void addref();
	void release();

	uint16_t node_type() const;

	std::string_view node_name() const;

	node parent_node() const;
	node first_child() const;
	node last_child() const;
	node previous_sibling() const;
	node next_sibling() const;

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

protected:
	explicit node_intf(uint16_t node_type, document_intf * document);
	virtual ~node_intf();

private:
	static void delete_tree(node_intf * n);

	size_t ref_count_ = 0;

	uint16_t type_;
	document_intf * document_;

	std::string node_name_;

	node_intf * parent_ = nullptr;

	node_intf * first_child_ = nullptr;
	node_intf * last_child_ = nullptr;

	node_intf * next_sibling_ = nullptr;
	node_intf * previous_sibling_ = nullptr;
};

struct element_intf
	: node_intf
{
	static constexpr uint16_t node_type_id = element_node;

	std::string_view tag_name() const;

	bool has_attribute(std::string_view name) const;
	std::string_view get_attribute(std::string_view name) const;
	void set_attribute(std::string_view name, std::string_view value);
	void remove_attribute(std::string_view name);

	explicit element_intf(std::string_view local_name, document_intf * doc);

private:
	std::string tag_name_;
	std::map<std::string, std::string, std::less<>> attributes_;
};

struct character_data_intf
	: node_intf
{
	std::string_view data() const;
	void data(std::string_view value);

	size_t length() const;

	explicit character_data_intf(uint16_t node_type, document_intf * doc);

private:
	std::string data_;
};

struct text_intf
	: character_data_intf
{
	static constexpr uint16_t node_type_id = text_node;

	std::string whole_text() const;

	explicit text_intf(document_intf * doc);
};

struct document_intf
	: node_intf
{
	static constexpr uint16_t node_type_id = document_node;

	document_intf();

	virtual element create_element(std::string_view local_name) = 0;
	virtual text create_text_node(std::string_view data) = 0;
};

template <typename D>
struct property_map_intf
{
	virtual typename D::element & get_element_data(element e) = 0;
	virtual typename D::text & get_text_data(text t) = 0;
};

template <typename D>
using property_map = std::shared_ptr<property_map_intf<D>>;

template <typename... Dn>
document create_document(property_map<Dn> &... pms);

}

#include "dom3_impl.h"
