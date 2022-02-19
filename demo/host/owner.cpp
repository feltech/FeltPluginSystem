#include "owner.hpp"
#include "feltplugindemohostlib_export.h"

#include <feltplugindemo/interface.h>

#include "handle_map.hpp"

extern "C"
{
	using String = feltplugindemohost::owner::String;
	using StringView = feltplugindemohost::owner::StringView;
	using StringDict = feltplugindemohost::owner::StringDict;

	// String

	using feltplugindemohost::owner::String;
	FELTPLUGINDEMOHOSTLIB_EXPORT fpdemo_String_s fpdemo_String_suite()
	{
		using HandleFactory = feltplugindemohost::owner::HandleFactory<fpdemo_String_h>;
		return {
			.create = &HandleFactory::make,

			.release = &HandleFactory::release,

			.assign_cstr =
				[](fp_ErrorMessage err, fpdemo_String_h hself, char const * cstr)
			{
				return HandleFactory::mem_fn(
					[](String & self, char const * str) {
						self = str;
					}, err, hself, cstr);
			},

			.assign_StringView =
				[](fp_ErrorMessage err, fpdemo_String_h hself, fpdemo_StringView_h hstr)
			{
				return HandleFactory::mem_fn(
					[](String & self, StringView const & str) { self = str; }, err, hself, hstr);
			},

			.c_str =
				[](fpdemo_String_h handle) {
					return HandleFactory::mem_fn(
						[](String const & self) { return self.c_str(); }, handle);
				},

			.at =
				[](fp_ErrorMessage err, char * out, fpdemo_String_h handle, int n)
			{
				return HandleFactory::mem_fn(
					[](String const & self, int n) { return self.at(n); }, err, out, handle, n);
			}};
	}

	// StringView

	using feltplugindemohost::owner::StringView;

	FELTPLUGINDEMOHOSTLIB_EXPORT fpdemo_StringView_s fpdemo_StringView_suite()
	{
		using HandleFactory = feltplugindemohost::owner::HandleFactory<fpdemo_StringView_h>;
		return {
			.data =
				[](fpdemo_StringView_h handle) {
					return HandleFactory::mem_fn(
						[](StringView const & self) { return self.data(); }, handle);
				},

			.size =
				[](fpdemo_StringView_h handle) {
					return HandleFactory::mem_fn(
						[](StringView const & self) { return self.size(); }, handle);
				}};
	}

	// StringDict

	FELTPLUGINDEMOHOSTLIB_EXPORT fp_StringDict_s fpdemo_StringDict_suite()
	{
		using HandleFactory = feltplugindemohost::owner::HandleFactory<fpdemo_StringDict_h>;
		return {
			.create = &HandleFactory::make,

			.release = &HandleFactory::release,

			.insert =
				[](fp_ErrorMessage err,
				   fpdemo_StringDict_h handle,
				   fpdemo_String_h key,
				   fpdemo_String_h value)
			{
				return HandleFactory::mem_fn(
					[](StringDict & self, String key, String value) {
						return self.insert_or_assign(std::move(key), std::move(value));
					},
					err,
					handle,
					key,
					value);
			},

			.at =
				[](fp_ErrorMessage err,
				   fpdemo_String_h * out,
				   fpdemo_StringDict_h handle,
				   fpdemo_String_h key)
			{
				return HandleFactory::mem_fn(
					[](StringDict const & self, String const & key) { return self.at(key); },
					err,
					out,
					handle,
					key);
			}};
	}
}
