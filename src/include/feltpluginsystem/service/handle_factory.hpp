// Copyright 2022 David Feltell
// SPDX-License-Identifier: MIT
/**
 * Contains the HandleFactory to be used by services converting to/from opaque handles.
 */
#pragma once

#include <cstring>
#include <functional>

#include "../error_map.hpp"
#include "../interface.h"
#include "../pointers.hpp"
#include "handle_map.hpp"

namespace feltplugin::service
{

/**
 * Utility class for creating and converting opaque handles, and adapting function pointer suites.
 *
 * @tparam THandle Opaque handle type.
 * @tparam TServiceHandleMap service::HandleMap for mapping handles to native classes.
 * @tparam TClientHandleMap client::HandleMap for mapping handles to client adapter classes.
 * @tparam TErrorMap ErrorMap for mapping exceptions to error codes.
 */
template <
	class THandle,
	class TServiceHandleMap,
	class TClientHandleMap,
	class TErrorMap = ErrorMap<>>
struct HandleFactory
{
private:
	template <class Handle>
	using OtherHandleFactory =
		HandleFactory<Handle, TServiceHandleMap, TClientHandleMap, TErrorMap>;

	using Handle = THandle;
	using Class = typename TServiceHandleMap::template class_from_handle<Handle>;
	using Adapter = typename TClientHandleMap::template class_from_handle<Handle>;
	static constexpr HandleOwnershipTag ptr_type_tag =
		TServiceHandleMap::template ownersihp_tag_from_handle<Handle>();

	/**
	 * Utility to static_assert that a given handle maps to either a native class or adapter.
	 *
	 * If the Handle type is not found in the service::HandleMap, then Class resolves to Handle
	 * (pass-through). If the Handle type is not found in the client::HandleMap, then the Adapter
	 * resolves to `std::false_type`.
	 *
	 * @tparam Handle Handle type to check.
	 * @tparam Class Native class type that the handle is associated with, if any.
	 * @tparam Adapter Adapter class that the handle is associated with, if any.
	 */
	template <class Handle, class Class, class Adapter>
	struct assert_is_valid_handle_type
	{
		static_assert(
			!(std::is_same_v<Handle, Class> && std::is_same_v<Adapter, std::false_type>),
			"Attempting to wrap a handle that is unrecognized. Are you missing an entry in your "
			"HandleMap lists?");
	};

	static_assert(
		std::is_same_v<Handle, Class> | std::is_same_v<Adapter, std::false_type>,
		"Cannot have a handle that is associated with both a native type and a client adapter. "
		"Check HandleMap lists.");

public:
	/**
	 * Construct a new instance of our Class type and associate it with a Handle.
	 *
	 * The signature of this function matches the convention that function pointer suites should
	 * adhere to, allowing it to be used directly without wrapping, e.g. when defining the `create`
	 * member during construction of a function pointer suite we can simply do
	 * `.create = &HandleFactory:::make,`.
	 *
	 * @tparam Args Argument types to pass to the constructor.
	 * @param err Storage for exception message, if one occurs during construction.
	 * @param[out] out Pointer to handle to newly constructed object.
	 * @param args Arguments to pass to the constructor.
	 * @return Error code.
	 */
	template <typename... Args>
	static fp_ErrorCode make(fp_ErrorMessage err, Handle * out, Args... args)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		return TErrorMap::wrap_exception(
			err, [&out, &args...] { *out = make(std::forward<Args>(args)...); });
	}

	/**
	 * Construct a new instance of our Class type and associate it with a Handle.
	 *
	 * Ownership is determined by the `ptr_type_tag` enum value in the HandleTraits for our Handle.
	 *
	 * This function is not valid if the HandlePtrTag is `OwnedByService`, since that implies
	 * a handle should be associated with an existing object rather than creating a new one.
	 *
	 * @tparam Args Argument types to pass to the constructor.
	 * @param args Arguments to pass to the constructor.
	 * @return Newly minted opaque handle.
	 */
	template <typename... Args>
	static Handle make(Args &&... args)
	{
		static_assert(
			ptr_type_tag != HandleOwnershipTag::OwnedByService,
			"Cannot make a handle to a new instance for non-shared non-transferred types");

		if constexpr (ptr_type_tag == HandleOwnershipTag::Shared)
		{
			return create(feltplugin::make_shared<Class>(std::forward<Args>(args)...));
		}
		else if constexpr (ptr_type_tag == HandleOwnershipTag::OwnedByClient)
		{
			return reinterpret_cast<Handle>(new Class{std::forward<Args>(args)...});
		}
		// Native type.
		else if constexpr (ptr_type_tag == HandleOwnershipTag::Unrecognized)
		{
			static_assert(
				std::is_same_v<Adapter, std::false_type>,
				"Reconstructing a handle when it should be wrapped.");
			return Class{std::forward<Args>(args)...};
		}
	}

	/**
	 * Create a handle associated with a pre-existing instance.
	 *
	 * This function is only valid if the HandlePtrTag in the HandleTraits is `OwnedByService`.
	 *
	 * @param obj Object to reference.
	 * @return Newly minted opaque handle.
	 */
	static Handle create(Class & obj)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		static_assert(
			ptr_type_tag == HandleOwnershipTag::OwnedByService,
			"Cannot create a non-shared non-transferred handle for shared / transferred types");
		return reinterpret_cast<Handle>(&obj);
	}

	/**
	 * Create a handle associated with a pre-existing shared smart pointer to an instance.
	 *
	 * This function is only valid if the `ptr_type_tag` in the HandleTraits is `Shared`.
	 *
	 * The given shared pointer's reference count will be incremented and not decremented again
	 * until `release` is called.
	 *
	 * @param ptr Pointer to wrap.
	 * @return Newly minted opaque handle.
	 */
	static Handle create(SharedPtr<Class> const & ptr)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		static_assert(
			ptr_type_tag == HandleOwnershipTag::Shared,
			"Cannot create a shared handle for a non-shared type");
		return reinterpret_cast<Handle>(new SharedPtr<Class>{ptr});
	}

	/**
	 * Create a handle associated with a pre-existing shared smart pointer to an instance.
	 *
	 * This function is only valid if the `ptr_type_tag` in the HandleTraits is `Shared`.
	 *
	 * The given shared pointer's reference count will be incremented and not decremented again
	 * until `release` is called.
	 *
	 * @param ptr Pointer to wrap.
	 * @return Newly minted opaque handle.
	 */
	static Handle create(SharedPtr<Class> && ptr)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		static_assert(
			ptr_type_tag == HandleOwnershipTag::Shared,
			"Cannot create a shared handle for a non-shared type");
		return reinterpret_cast<Handle>(new SharedPtr<Class>{ptr});
	}

	/**
	 * Release an opaque handle.
	 *
	 * This function is only valid if the `ptr_type_tag` in our HandleTraits is `OwnedByClient` or
	 * `Shared`.
	 *
	 * If `OwnedByClient` then the object is destroyed. If `Shared` then the reference count is
	 * decremented, potentially destroying the object.
	 *
	 * @param handle Handle to release.
	 */
	static void release(Handle handle)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		static_assert(
			ptr_type_tag != HandleOwnershipTag::OwnedByService,
			"Cannot release a handle not owned by client");
		static_assert(
			ptr_type_tag != HandleOwnershipTag::Unrecognized,
			"Cannot release a handle aliasing a temporary. Are you missing an entry in your "
			"HandleMaps?");

		if constexpr (ptr_type_tag == HandleOwnershipTag::Shared)
		{
			delete reinterpret_cast<SharedPtr<Class> *>(handle);
		}
		else if constexpr (ptr_type_tag == HandleOwnershipTag::OwnedByClient)
		{
			delete reinterpret_cast<Class *>(handle);
		}
	}

	/**
	 * Convert an opaque handle to a concrete instance.
	 *
	 * The behaviour of this function varies depending on `ptr_type_tag` in our HandleTraits.
	 * However, a dereferencable object is always returned, which when dereferenced yields the
	 * concrete object. This can be either the original object that is associated with the handle,
	 * or a client adapter class that wraps the handle.
	 *
	 * @tparam Handle Type of handle. Required to enable forwarding references.
	 * @param handle Opaque handle to convert.
	 * @return Dereferenceable object that dereferences to the original object associated with the
	 * opaque handle.
	 */
	template <class Handle>
	static decltype(auto) convert(Handle && handle)
	{
		if constexpr (
			ptr_type_tag == HandleOwnershipTag::OwnedByClient ||
			ptr_type_tag == HandleOwnershipTag::OwnedByService)
		{
			return reinterpret_cast<Class *>(handle);
		}
		else if constexpr (ptr_type_tag == HandleOwnershipTag::Shared)
		{
			return *reinterpret_cast<SharedPtr<Class> *>(handle);
		}
		else if constexpr (ptr_type_tag == HandleOwnershipTag::Unrecognized)
		{
			if constexpr (std::is_same_v<Adapter, std::false_type>)
			{
				// Native C type.
				return &handle;
			}
			else
			{
				// Client handle type. Create adapter object and return a dereferencable object that
				// resolves to the adapter. Messy, but necessary to be consistent with other code
				// paths.
				return Dereferenceable<Adapter>{Adapter{handle}};
			}
		}
	}

	/**
	 * Adapt a suite function to have a more C++-like interface, automatically converting handles.
	 *
	 * The arguments will be converted from handles to objects using `convert`, and the return value
	 * converted from an object to a handle using `make`, as appropriate.
	 *
	 * @tparam Ret Return type of suite function.
	 * @tparam Fn Type of wrapped callable.
	 * @tparam Args Argument types to convert then pass to wrapped callable.
	 * @param fn Wrapped callable to execute.
	 * @param[out] err Storage for exception message, if any.
	 * @param[out] out Return value destination.
	 * @param handle Opaque handle to self.
	 * @param args Arguments to pass along to wrapped callable, converting from opaque handles to
	 * concrete types if necessary.
	 * @return Error code.
	 */
	template <class Ret, class Fn, class... Args>
	static fp_ErrorCode mem_fn(
		Fn && fn, fp_ErrorMessage err, Ret * out, Handle handle, Args... args)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		return TErrorMap::wrap_exception(
			err,
			[handle, &out, &fn, &args...]
			{
				*out = OtherHandleFactory<Ret>::make(
					fn(*convert(handle),
					   *OtherHandleFactory<Args>::convert(std::forward<Args>(args))...));
			});
	}

	/**
	 * Adapt a suite function that has no return value.
	 *
	 * Gives a more C++-like interface, automatically converting handles.
	 *
	 * The arguments will be converted from handles to objects using `convert`.
	 *
	 * @tparam Fn Type of wrapped callable.
	 * @tparam Args Argument types to convert then pass to wrapped callable.
	 * @param fn Wrapped callable to execute.
	 * @param[out] err Storage for exception message, if any.
	 * @param handle Opaque handle to self.
	 * @param args Arguments to pass along to wrapped callable, converting from opaque handles to
	 * concrete types if necessary.
	 * @return Error code.
	 */
	template <class Fn, class... Args>
	static fp_ErrorCode mem_fn(Fn && fn, fp_ErrorMessage err, Handle handle, Args... args)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		return TErrorMap::wrap_exception(
			err,
			[&fn, handle, &args...] {
				fn(*convert(handle),
				   *OtherHandleFactory<Args>::convert(std::forward<Args>(args))...);
			});
	}

	/**
	 * Adapt a suite function that has no return value and does not throw exceptions.
	 *
	 * Gives a more C++-like interface, automatically converting handles.
	 *
	 * The arguments will be converted from handles to objects using `convert`.
	 *
	 * @tparam Fn Type of wrapped callable.
	 * @tparam Args Argument types to convert then pass to wrapped callable.
	 * @param fn Wrapped callable to execute.
	 * @param[out] err Storage for exception message, if any.
	 * @param handle Opaque handle to self.
	 * @param args Arguments to pass along to wrapped callable, converting from opaque handles to
	 * concrete types if necessary.
	 * @return Error code.
	 */
	template <class Fn, class... Args>
	static auto mem_fn(Fn && fn, Handle handle, Args... args)
	{
		assert_is_valid_handle_type<Handle, Class, Adapter>{};
		return fn(
			*convert(handle), *OtherHandleFactory<Args>::convert(std::forward<Args>(args))...);
	}

private:
	template <class T>
	struct Dereferenceable
	{
		Dereferenceable(Dereferenceable const &) = delete;
		T t;
		T operator*() &&
		{
			return std::move(t);
		}
	};
};

}  // namespace feltplugin::service