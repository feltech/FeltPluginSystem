#pragma once

#include <type_traits>

#include "pointers.hpp"

namespace feltplugin
{

template <class THandle, class TPtr, class TClass>
struct HandleTraits
{
	using Handle = THandle;
	using Ptr = TPtr;
	using Class = TClass;
};

template <class Traits, class... Rest>
struct HandleMap
{
	template <class HandleToLookup>
	using ptr_from_handle_t = typename std::disjunction<
		typename HandleMap<Traits>::template ptr_from_handle_t<HandleToLookup>,
		typename HandleMap<Rest...>::template ptr_from_handle_t<HandleToLookup>>;

	template <class HandleToLookup>
	using class_from_handle_t = typename std::disjunction<
		typename HandleMap<Traits>::template class_from_handle_t<HandleToLookup>,
		typename HandleMap<Rest...>::template class_from_handle_t<HandleToLookup>>;

	template <class HandleToLookup>
	using ptr_from_handle = typename ptr_from_handle_t<HandleToLookup>::type;

	template <class HandleToLookup>
	using class_from_handle = typename class_from_handle_t<HandleToLookup>::type;
};

template <class Traits>
struct HandleMap<Traits>
{
	using Handle =  typename Traits::Handle;
	using Ptr =  typename Traits::Ptr;
	using Class =  typename Traits::Class;

	template <class Other>
	struct ptr_from_handle_t : std::is_same<Handle, Other>
	{
		using type = Ptr;
	};

	template <class Other>
	struct class_from_handle_t : std::is_same<Handle, Other>
	{
		using type = Class;
	};

	template <class Arg>
	using ptr_from_handle = typename ptr_from_handle_t<Arg>::type;

	template <class Arg>
	using class_from_handle = typename class_from_handle_t<Arg>::type;
};
}