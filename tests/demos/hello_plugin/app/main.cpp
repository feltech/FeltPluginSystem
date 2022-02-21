// Copyright 2022 David Feltell
// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <feltpluginsystem/handle_wrapper.hpp>
#include <feltpluginsystem/plugin.hpp>

#include <feltpluginsystem-demo-hello_plugin/interface.h>

struct Worker;

// Define C <-> C++ interface.
using HandleWrapper = feltplugin::HandleWrapper<
	// Service - nothing offered.
	feltplugin::service::HandleMap<>,

	// Client
	feltplugin::client::HandleMap<feltplugin::client::HandleMap<
		// Worker
		feltplugin::client::HandleTraits<fpdemo_Worker_h, fpdemo_Worker_s, Worker>>>>;

struct Worker : HandleWrapper::Adapter<fpdemo_Worker_h>
{
	using Base::HandleAdapter;

	void work()
	{
		call(suite_.work);
	}
};

int main()
{
	// Calculate plugin path.
	char const * plugin_path_var = std::getenv("FELTPLUGINSYSTEM_PLUGIN_PATH");
	if (!plugin_path_var)
		throw std::invalid_argument{"FELTPLUGINSYSTEM_PLUGIN_PATH must be set"};
	std::filesystem::path plugin_path{plugin_path_var};

	plugin_path /= "libfeltpluginsystem-demo-hello_plugin-plugin.so";
	std::cout << "Loading plugin at " << plugin_path << std::endl;

	// Load the plugin DSO.
	feltplugin::Plugin plugin{plugin_path.c_str()};

	auto worker = plugin.load_adapter<Worker>("fpdemo_Worker_suite");

	std::cout << "Host client telling plugin service to do work..." << std::endl;

	worker.work();

	return 0;
}