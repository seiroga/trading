#pragma once

#include <core/primitives.h>
#include <core/data_storage.h>
#include <core/data_collector.h>
#include <core/connector.h>

namespace tbp
{
	struct factory
	{
	public:
		using ptr = std::shared_ptr<factory>;

	public:
		virtual authentication::ptr create_auth() = 0;
		virtual connector::ptr create_connector(const authentication::ptr& auth) = 0;
		virtual data_storage::ptr create_storage() = 0;
	};
}