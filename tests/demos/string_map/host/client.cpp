// Copyright 2022 David Feltell
// SPDX-License-Identifier: MIT
#include "client.hpp"
#include "service.hpp"

namespace feltpluginsystemdemohost::client
{

Worker::Worker(SuiteFactory suite_factory, feltplugin::SharedPtr<service::StringDict> dict)
	: Base{suite_factory}
{
	create(std::move(dict));
}

void Worker::update_dict(service::StringView key)
{
	call(suite_.update_dict, key);
}
}  // namespace feltpluginsystemdemohost::client
