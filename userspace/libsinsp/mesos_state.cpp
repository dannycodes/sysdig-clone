//
// k8s_state.cpp
//

#include "mesos_state.h"
#include "sinsp.h"
#include "sinsp_int.h"
#include <sstream>
#include <iostream>
#include <memory>

//
// state
//

mesos_state_t::mesos_state_t(bool is_captured) : m_is_captured(is_captured)
{
}

mesos_framework::task_ptr_t mesos_state_t::get_task(const std::string& uid)
{
	for(auto& framework : get_frameworks())
	{
		for(auto& task : framework.get_tasks())
		{
			if(task.first == uid)
			{
				return task.second;
			}
		}
	}
	g_logger.log("Task not found: " + uid, sinsp_logger::SEV_WARNING);
	return 0;
}

std::unordered_set<std::string> mesos_state_t::get_all_task_ids() const
{
	std::unordered_set<std::string> tasks;
	for(const auto& framework : m_frameworks)
	{
		for(const auto& task : framework.get_tasks())
		{
			tasks.insert(task.first);
		}
	}
	return tasks;
}

const mesos_framework::task_map& mesos_state_t::get_tasks(const std::string& framework_uid) const
{
	for(const auto& framework : m_frameworks)
	{
		if(framework.get_uid() == framework_uid)
		{
			return framework.get_tasks();
		}
	}
	throw sinsp_exception("Framework not found: " + framework_uid);
}

mesos_framework::task_map& mesos_state_t::get_tasks(const std::string& framework_uid)
{
	for(auto& framework : m_frameworks)
	{
		if(framework.get_uid() == framework_uid)
		{
			return framework.get_tasks();
		}
	}
	throw sinsp_exception("Framework not found: " + framework_uid);
}

marathon_app::ptr_t mesos_state_t::get_app(const std::string& app_id)
{
	marathon_group::ptr_t group = get_app_group(app_id);
	if(group)
	{
		g_logger.log("Found group for app [" + app_id + "]: " + group->get_id(), sinsp_logger::SEV_DEBUG);
		return group->get_app(app_id);
	}
	return 0;
}

marathon_group::app_ptr_t mesos_state_t::add_or_replace_app(const std::string& app_id,
															const std::string& group_id,
															const std::string& task_id)
{
	marathon_group::app_ptr_t app = get_app(app_id);
	if(!app)
	{
		app = std::make_shared<marathon_app>(app_id);
		g_logger.log("Created app [" + app_id + ']', sinsp_logger::SEV_DEBUG);
	}
	else
	{
		g_logger.log("Found app [" + app_id + ']', sinsp_logger::SEV_DEBUG);
	}
	if(!app)
	{
		g_logger.log("Could not find or create app [" + app_id + ']', sinsp_logger::SEV_ERROR);
		return 0;
	}

	if(!task_id.empty())
	{
		g_logger.log("Adding task [" + task_id + "] to app [" + app_id + ']', sinsp_logger::SEV_DEBUG);
		add_task_to_app(app, task_id);
	}

	marathon_group::ptr_t group = get_group(group_id);
	if(group)
	{
		g_logger.log("Adding app [" + app_id + "] to group [" + group_id + ']', sinsp_logger::SEV_DEBUG);
		group->add_or_replace_app(app);
	}

	return app;
}

void mesos_state_t::add_task_to_app(marathon_group::app_ptr_t app, const std::string& task_id)
{
	if(app)
	{
		mesos_framework::task_ptr_t pt = get_task(task_id);
		if(pt)
		{
			app->add_task(pt);
		}
		else
		{
			g_logger.log("Task [" + task_id + "] can not be obtained (null). Task not added to app [" + app->get_id() + ']', sinsp_logger::SEV_ERROR);
		}
	}
	else
	{
		g_logger.log("Attempt to add task [" + task_id + "] to non-existing (null) app.", sinsp_logger::SEV_ERROR);
	}
}

marathon_group::ptr_t mesos_state_t::get_app_group(const std::string& app_id)
{
	std::string group_id = marathon_app::get_group_id(app_id);
	if(!group_id.empty())
	{
		return get_group(group_id);
	}
	return 0;
}

bool mesos_state_t::remove_app(const std::string& app_id)
{
	marathon_group::ptr_t group = get_group(app_id);
	if(group)
	{
		return group->remove_app(app_id);
	}
	return false;
}

marathon_group::ptr_t mesos_state_t::get_group(const std::string& group_id)
{
	marathon_groups::iterator it = m_groups.find(group_id);
	if(it != m_groups.end())
	{
		return it->second;
	}
	else
	{
		for(auto group : m_groups)
		{
			if(marathon_group::ptr_t p_group = group.second->get_group(group_id))
			{
				return p_group;
			}
		}
	}
	return 0;
}

marathon_group::ptr_t mesos_state_t::add_or_replace_group(marathon_group::ptr_t group, marathon_group::ptr_t to_group)
{
	std::string id = group->get_id();
	if(!to_group) // top level
	{
		marathon_groups::iterator it = m_groups.find(id);
		if(it != m_groups.end())
		{
			m_groups.erase(it);
		}
		m_groups.insert({id, group});
	}
	else
	{
		to_group->add_or_replace_group(group);
	}
	return group;
}

bool mesos_state_t::handle_groups(const Json::Value& root, marathon_group::ptr_t to_group, const std::string& framework_id)
{
	Json::Value groups = root["groups"];
	if(!groups.isNull() && groups.isArray())
	{
		for(const auto& group : groups)
		{
			add_group(group, to_group, framework_id);
		}
	}
	else
	{
		g_logger.log("No groups found.", sinsp_logger::SEV_WARNING);
		return false;
	}
	return true;
}

bool mesos_state_t::parse_groups(std::string&& json, const std::string& framework_id)
{
	Json::Value root;
	Json::Reader reader;
	if(reader.parse(json, root, false) && !root["id"].isNull())
	{
		add_group(root, 0, framework_id);
		return true;
	}
	else
	{
		throw sinsp_exception("Marathon groups parsing failed (Invalid JSON).");
	}
}

void mesos_state_t::erase_groups(const std::string& framework_id)
{
	for(marathon_groups::iterator it = m_groups.begin(); it != m_groups.end();)
	{
		if(it->second->get_framework_id() == framework_id)
		{
			m_groups.erase(it++);
		}
		else { ++it; }
	}
}

void mesos_state_t::print_groups() const
{
	for(auto& group : m_groups)
	{
		group.second->print();
	}
}

marathon_group::ptr_t mesos_state_t::add_group(const Json::Value& group, marathon_group::ptr_t to_group, const std::string& framework_id)
{
	const Json::Value& group_id = group["id"];
	if(!group_id.isNull())
	{
		std::string id = group_id.asString();
		std::ostringstream os;
		os << "Adding Marathon group [" << id << ']';
		if(to_group)
		{
			os << " to group [" << to_group->get_id() << ']';
		}
		g_logger.log(os.str(), sinsp_logger::SEV_DEBUG);

		marathon_group::ptr_t pg(new marathon_group(id, framework_id));
		add_or_replace_group(pg, to_group);

		const Json::Value& apps = group["apps"];
		if(!apps.isNull())
		{
			for(const auto& app : apps)
			{
				const Json::Value& app_id = app["id"];
				if(!app_id.isNull())
				{
					const Json::Value& instances = app["instances"];
					if(!instances.isNull() && instances.isInt() && instances.asInt() > 0)
					{
						marathon_app::ptr_t p_app = get_app(app_id.asString());
						if(!p_app)
						{
							p_app = add_app(app, framework_id);
						}
						if(p_app)
						{
							pg->add_or_replace_app(p_app);
							if(!framework_id.empty())
							{
								for(const auto& task : get_tasks(framework_id))
								{
									if(task.second->get_marathon_app_id() == app_id.asString())
									{
										add_task_to_app(p_app, task.first);
									}
								}
							}
						}
						else
						{
							g_logger.log("An error occured adding app [" + app_id.asString() +
										"] to group [" + id + ']', sinsp_logger::SEV_ERROR);
						}
					}
				}
			}
		}

		Json::Value groups = group["groups"];
		if(!groups.isNull() && groups.isArray())
		{
			handle_groups(group, pg, framework_id);
		}
		return pg;
	}
	return 0;
}

void mesos_state_t::parse_apps(std::string&& json, const std::string& framework_id)
{
	Json::Value root;
	Json::Reader reader;
	if(reader.parse(json, root, false))
	{
		Json::Value apps = root["apps"];
		if(!apps.isNull())
		{
			for(const auto& app : apps)
			{
				add_app(app, framework_id);
			}
		}
		else
		{
			g_logger.log("No apps found.", sinsp_logger::SEV_WARNING);
		}
	}
	else
	{
		g_logger.log(json, sinsp_logger::SEV_DEBUG);
		throw sinsp_exception("Invalid JSON (Marathon apps parsing failed).");
	}
}

marathon_app::ptr_t mesos_state_t::add_app(const Json::Value& app, const std::string& /*framework_id*/)
{
	marathon_app::ptr_t p_app = 0;
	Json::Value app_id = app["id"];
	if(!app_id.isNull())
	{
		std::string id = app_id.asString();
		g_logger.log("Adding Marathon app: " + id, sinsp_logger::SEV_DEBUG);
		std::string group_id = marathon_app::get_group_id(id);
		if(!group_id.empty())
		{
			p_app = add_or_replace_app(id, group_id);
			if(p_app)
			{
				g_logger.log("Added app [" + id + "] to Marathon group: [" + group_id + ']', sinsp_logger::SEV_DEBUG);
				Json::Value tasks = app["tasks"];
				if(tasks.size())
				{
					g_logger.log("App [" + id + "] has " + std::to_string(tasks.size()) + " tasks.", sinsp_logger::SEV_DEBUG);
					for(const auto& task : tasks)
					{
						Json::Value task_id = task["id"];
						if(!task_id.isNull())
						{
							std::string tid = task_id.asString();
							g_logger.log("Adding Mesos task ID to app [" + id + "]: " + tid, sinsp_logger::SEV_DEBUG);
							mesos_framework::task_ptr_t pt = get_task(task_id.asString());
							if(pt)
							{
								pt->set_marathon_app_id(id);
								add_task_to_app(p_app, tid);
							}
							else
							{
								g_logger.log("Marathon task not found in mesos state: " + tid, sinsp_logger::SEV_WARNING);
							}
						}
					}
				}
			}
			else
			{
				g_logger.log("NOT added app [" + id + "] to Marathon group: [" + group_id + ']', sinsp_logger::SEV_ERROR);
			}
		}
		else
		{
			g_logger.log("Could not determine group ID for app: " + id, sinsp_logger::SEV_ERROR);
		}
	}
	return p_app;
}

