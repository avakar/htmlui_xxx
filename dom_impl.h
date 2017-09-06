#include <assert.h>

namespace dom3 {

template <typename T>
node_ptr<T>::node_ptr()
	: ptr_(nullptr)
{
}

template <typename T>
node_ptr<T>::node_ptr(T * ptr)
	: ptr_(ptr)
{
	if (ptr_)
		ptr_->addref();
}

template <typename T>
template <typename U>
node_ptr<T>::node_ptr(node_ptr<U> const & o)
	: ptr_(o.get())
{
	if (ptr_)
		ptr_->addref();
}

template <typename T>
template <typename U>
node_ptr<T>::node_ptr(node_ptr<U> && o)
	: ptr_(o.release())
{
}

template <typename T>
node_ptr<T>::node_ptr(node_ptr const & o)
	: ptr_(o.ptr_)
{
	if (ptr_)
		ptr_->addref();
}

template <typename T>
node_ptr<T>::node_ptr(node_ptr && o)
	: ptr_(o.ptr_)
{
	o.ptr_ = nullptr;
}

template <typename T>
node_ptr<T> & node_ptr<T>::operator=(node_ptr o)
{
	std::swap(ptr_, o.ptr_);
	return *this;
}

template <typename T>
node_ptr<T>::~node_ptr()
{
	if (ptr_)
		ptr_->release();
}

template <typename T>
T * node_ptr<T>::release()
{
	T * ptr = ptr_;
	ptr_ = nullptr;
	return ptr;
}

template <typename T>
void node_ptr<T>::reset(T * ptr)
{
	if (ptr)
		ptr->addref();
	if (ptr_)
		ptr_->release();
	ptr_ = ptr;
}

template <typename T>
node_ptr<T>::operator bool() const
{
	return ptr_ != nullptr;
}

template <typename T>
T * node_ptr<T>::get() const
{
	return ptr_;
}

template <typename T>
T & node_ptr<T>::operator*() const
{
	assert(ptr_ != nullptr);
	return *ptr_;
}

template <typename T>
T * node_ptr<T>::operator->() const
{
	assert(ptr_ != nullptr);
	return ptr_;
}

template <typename T>
template <typename U>
std::enable_if_t<is_node<U>::value, U> node_ptr<T>::cast()
{
	using I = typename U::value_type;

	if (!ptr_ || ptr_->node_type() != I::node_type_id)
		return U();

	return U(static_cast<I *>(ptr_));
}

template <typename T>
bool node_ptr<T>::operator==(node_ptr const & rhs) const
{
	return ptr_ == rhs.ptr_;
}

template <typename T>
bool node_ptr<T>::operator!=(node_ptr const & rhs) const
{
	return !(*this == rhs);
}

template <typename... Dn>
struct _element_node final
	: element_intf
{
	using element_intf::element_intf;

	std::tuple<typename Dn::element...> extra;
};

template <typename... Dn>
struct _text_node final
	: text_intf
{
	using text_intf::text_intf;

	std::tuple<typename Dn::text...> extra;
};

template <size_t I, typename... Dn>
struct _property_map_impl final
	: property_map_intf<std::tuple_element_t<I, std::tuple<Dn...>>>
{
	using D = std::tuple_element_t<I, std::tuple<Dn...>>;

	document doc;

	_property_map_impl(document doc)
		: doc(std::move(doc))
	{
	}

	typename D::element & get_element_data(element e) override
	{
		assert(e->owner_document() == doc);
		return std::get<I>(static_cast<_element_node<Dn...> *>(e.get())->extra);
	}

	typename D::text & get_text_data(text t) override
	{
		assert(t->owner_document() == doc);
		return std::get<I>(static_cast<_text_node<Dn...> *>(t.get())->extra);
	}
};

template <typename... Dn>
struct _document_impl final
	: document_intf
{
	element create_element(std::string_view local_name) override
	{
		return element(new _element_node<Dn...>(local_name, this));
	}

	text create_text_node(std::string_view data) override
	{
		text n(new _text_node<Dn...>(this));
		n->data(data);
		return n;
	}
};

template <typename... Dn>
struct _document_factory
{
	template <size_t I>
	static void create_pms(document const &)
	{
	}

	template <size_t I, typename A0, typename... An>
	static void create_pms(document const & doc, property_map<A0> & a0, property_map<An> &... an)
	{
		a0.reset(new _property_map_impl<I, Dn...>(doc));
		create_pms<I + 1>(doc, an...);
	}
};

template <typename... Dn>
document create_document(property_map<Dn> &... pms)
{
	document doc_ptr(new _document_impl<Dn...>());
	_document_factory<Dn...>::create_pms<0>(doc_ptr, pms...);
	return doc_ptr;
}

}
