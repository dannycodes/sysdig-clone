//
// mesos_state_t.h
//
// mesos state abstraction
//

#pragma once

#include "mesos_component.h"
#include "marathon_component.h"
#include "json/json.h"
#include "sinsp.h"
#include "sinsp_int.h"
#include <vector>
#include <map>
#include <unordered_map>

//
// state
//

class mesos_state_t
{
public:
	mesos_state_t(bool is_captured = false);

	//
	// frameworks
	//
	const mesos_frameworks& get_frameworks() const;
	mesos_frameworks& get_frameworks();
	const mesos_framework& get_framework(const std::string& framework_uid) const;
	mesos_framework& get_framework(const std::string& framework_uid);
	void push_framework(const mesos_framework& framework);
	void emplace_framework(mesos_framework&& framework);
	void remove_framework(const std::string& framework_uid);
	void remove_framework(const Json::Value& framework);

	//
	// tasks
	//
	std::unordered_set<std::string> get_all_task_ids() const;
	const mesos_framework::task_map& get_tasks(const std::string& framework_uid) const;
	mesos_framework::task_map& get_tasks(const std::string& framework_uid);
	mesos_framework::task_ptr_t get_task(const std::string& uid);
	void add_or_replace_task(mesos_framework& framework, mesos_task::ptr_t task);
	void remove_task(mesos_framework& framework, const std::string& uid);

	//
	// slaves
	//
	const mesos_slaves& get_slaves() const;
	mesos_slaves& get_slaves();
	const mesos_slave& get_slave(const std::string& slave_uid) const;
	mesos_slave& get_slave(const std::string& slave_uid);
	void push_slave(const mesos_slave& slave);
	void emplace_slave(mesos_slave&& slave);

	//
	// Marathon
	//

	//
	// Marathon apps
	//
	void parse_apps(std::string&& json, const std::string& framework_id);
	marathon_app::ptr_t get_app(const std::string& app_id);
	marathon_group::app_ptr_t add_or_replace_app(const std::string& id,
												const std::string& group,
												const std::string& task = "");
	bool remove_app(const std::string& id);
	void add_task_to_app(marathon_group::app_ptr_t app, const std::string& task_id);

	//
	// Marathon groups
	//
	bool parse_groups(std::string&& json, const std::string& framework_id);
	const marathon_groups& get_groups() const;
	marathon_groups& get_groups();
	marathon_group::ptr_t get_group(const std::string& group_id);
	marathon_group::ptr_t add_or_replace_group(marathon_group::ptr_t group, marathon_group::ptr_t to_group = 0);
	marathon_group::ptr_t get_app_group(const std::string& app_id);
	void erase_groups(const std::string& framework_id);
	void print_groups() const;

	//
	// state
	//
	void clear_mesos();
	void clear_marathon();
	bool has_data() const;

private:
	marathon_group::ptr_t add_group(const Json::Value& group, marathon_group::ptr_t to_group, const std::string& framework_id);
	bool handle_groups(const Json::Value& groups, marathon_group::ptr_t p_groups, const std::string& framework_id);
	marathon_app::ptr_t add_app(const Json::Value& app, const std::string& framework_id);

	mesos_frameworks m_frameworks;
	mesos_slaves     m_slaves;
	marathon_groups  m_groups;
	bool             m_is_captured;

	std::unordered_multimap<std::string, std::string> m_marathon_task_cache;
};

//
// frameworks
//

inline const mesos_frameworks& mesos_state_t::get_frameworks() const
{
	return m_frameworks;
}

inline mesos_frameworks& mesos_state_t::get_frameworks()
{
	return m_frameworks;
}

inline const mesos_framework& mesos_state_t::get_framework(const std::string& framework_uid) const
{
	for(const auto& framework : m_frameworks)
	{
		if(framework.get_uid() == framework_uid)
		{
			return framework;
		}
	}
	throw sinsp_exception("Framework not found: " + framework_uid);
}

inline mesos_framework& mesos_state_t::get_framework(const std::string& framework_uid)
{
	for(auto& framework : m_frameworks)
	{
		if(framework.get_uid() == framework_uid)
		{
			return framework;
		}
	}
	throw sinsp_exception("Framework not found: " + framework_uid);
}

inline void mesos_state_t::push_framework(const mesos_framework& framework)
{
	for(mesos_frameworks::iterator it = m_frameworks.begin(); it != m_frameworks.end();)
	{
		if(it->get_uid() == framework.get_uid())
		{
			it = m_frameworks.erase(it);
		}
		else { ++it; }
	}
	m_frameworks.push_back(framework);
}

inline void mesos_state_t::emplace_framework(mesos_framework&& framework)
{
	for(mesos_frameworks::iterator it = m_frameworks.begin(); it != m_frameworks.end();)
	{
		if(it->get_uid() == framework.get_uid())
		{
			it = m_frameworks.erase(it);
		}
		else { ++it; }
	}
	m_frameworks.emplace_back(std::move(framework));
}

inline void mesos_state_t::remove_framework(const Json::Value& framework)
{
	const Json::Value& id = framework["id"];
	if(!id.isNull() && id.isString())
	{
		remove_framework(id.asString());
	}
}

inline void mesos_state_t::remove_framework(const std::string& framework_uid)
{
	for(mesos_frameworks::iterator it = m_frameworks.begin(); it != m_frameworks.end();)
	{
		if(it->get_uid() == framework_uid)
		{
			m_frameworks.erase(it);
			return;
		}
	}
}

//
// tasks
//
inline void mesos_state_t::add_or_replace_task(mesos_framework& framework, mesos_task::ptr_t task)
{
	framework.add_or_replace_task(task);
}

inline void mesos_state_t::remove_task(mesos_framework& framework, const std::string& uid)
{
	mesos_task::ptr_t task = framework.get_task(uid);
	if(task)
	{
		std::string app_id = task->get_marathon_app_id();
		if(!app_id.empty())
		{
			marathon_group::ptr_t group = get_app_group(app_id);
			if(group)
			{
				if(!group->remove_task(uid))
				{
					g_logger.log("Task [" + uid + "] not found in Marathon app [" + app_id + ']',
							 sinsp_logger::SEV_ERROR);
				}
			}
			else
			{
				g_logger.log("Group not found for Marathon app [" + app_id + "] while trying to remove task [" + uid + ']',
							 sinsp_logger::SEV_ERROR);
			}
		}
		else
		{
			g_logger.log("Task [" + uid + "] has no Marathon app ID.", sinsp_logger::SEV_WARNING);
		}
	}
	else
	{
		g_logger.log("Task [" + uid + "] not found in framework [" + framework.get_uid() + ']', sinsp_logger::SEV_WARNING);
	}
	framework.remove_task(uid);
}

//
// slaves
//

inline const mesos_slaves& mesos_state_t::get_slaves() const
{
	return m_slaves;
}

inline mesos_slaves& mesos_state_t::get_slaves()
{
	return m_slaves;
}

inline const mesos_slave& mesos_state_t::get_slave(const std::string& slave_uid) const
{
	for(const auto& slave : m_slaves)
	{
		if(slave.get_uid() == slave_uid)
		{
			return slave;
		}
	}
	throw sinsp_exception("Slave not found: " + slave_uid);
}

inline mesos_slave& mesos_state_t::get_slave(const std::string& slave_uid)
{
	for(auto& slave : m_slaves)
	{
		if(slave.get_uid() == slave_uid)
		{
			return slave;
		}
	}
	throw sinsp_exception("Slave not found: " + slave_uid);
}

inline void mesos_state_t::push_slave(const mesos_slave& slave)
{
	for(mesos_slaves::iterator it = m_slaves.begin(); it != m_slaves.end();)
	{
		if(it->get_uid() == slave.get_uid())
		{
			it = m_slaves.erase(it);
		}
		else { ++it; }
	}
	m_slaves.push_back(slave);
}

inline void mesos_state_t::emplace_slave(mesos_slave&& slave)
{
	for(mesos_slaves::iterator it = m_slaves.begin(); it != m_slaves.end();)
	{
		if(it->get_uid() == slave.get_uid())
		{
			it = m_slaves.erase(it);
		}
		else { ++it; }
	}
	m_slaves.emplace_back(std::move(slave));
}

//
// apps
//


//
// groups
//

inline const marathon_groups& mesos_state_t::get_groups() const
{
	return m_groups;
}

inline marathon_groups& mesos_state_t::get_groups()
{
	return m_groups;
}

//
// state
//

inline void mesos_state_t::clear_mesos()
{
	m_frameworks.clear();
	m_slaves.clear();
}

inline void mesos_state_t::clear_marathon()
{
	m_groups.clear();
}

inline bool mesos_state_t::has_data() const
{
	return m_frameworks.size() > 0 && m_slaves.size() > 0;
}
