#include <sqlite/sqlite.h>
#include <logging/log.h>

#include <win/fs.h>

#include <boost/variant.hpp>
#include <chrono>

namespace tbp
{
	struct data_t;

	using byte_t = unsigned char;
	using time_t = std::chrono::system_clock::time_point;
	using binary_t = std::vector<byte_t>;
	using value_t = boost::variant<std::wstring, time_t, bool, binary_t, data_t>;
	
	struct data_t : public std::map<std::wstring, value_t>
	{
		using ptr = std::shared_ptr<data_t>;

	public:
		using  std::map<std::wstring, value_t>::map;
	};

	struct data_provider
	{
		using ptr = std::shared_ptr<data_provider>;

	public:
		virtual std::vector<data_t::ptr> get_instrument_data(const std::wstring& instrument_id, time_t start_datetime, time_t end_datetime) = 0;
	};

	struct order
	{
		using ptr = std::shared_ptr<order>;

	public:
		virtual void execute() = 0;
	};

	class connector
	{
	public:
		using ptr = std::shared_ptr<connector>;

	public:
		virtual data_t::ptr get_instrument_data(const std::wstring& instrument_id) = 0;
		virtual order::ptr create_order() = 0;
	};

	class data_collector
	{
	public:
		virtual data_provider::ptr create_data_provider() = 0;
	};
}

int wmain()
{
	using win::fs::operator/;

	logging::init(logging::level::debug, win::fs::get_current_module_dir() / L"Logs", true);
	
	LOG_INFO << "Application started";
	LOG_INFO << "Application shutdown";

    return 0;
}