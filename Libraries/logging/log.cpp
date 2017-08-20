#include <logging/log.h>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/core/null_deleter.hpp>

namespace logging
{
	void init(level l, const std::wstring& path, bool replicate_on_cout)
	{
		boost::log::add_file_log
		(
			boost::log::keywords::file_name = path + L"log_%N.txt",
			boost::log::keywords::rotation_size = 1 * 1024 * 1024,
			boost::log::keywords::format = "[%TimeStamp%][P:%ProcessID%][T:%ThreadID%][%Severity%][L:%LineID%]: %Message%"
		);

		boost::log::add_common_attributes();

		boost::log::trivial::severity_level boost_level = boost::log::trivial::severity_level::info;
		switch (l)
		{
		case level::debug:
			boost_level = boost::log::trivial::severity_level::debug;
			break;

		case level::warning:
			boost_level = boost::log::trivial::severity_level::warning;
			break;

		case level::error:
			boost_level = boost::log::trivial::severity_level::error;
			break;

		case level::info:
			boost_level = boost::log::trivial::severity_level::info;
			break;
		}

		boost::log::core::get()->set_filter
		(
			boost::log::trivial::severity >= boost_level
		);

		// add sink on std::cout
		if (replicate_on_cout)
		{
			typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_sink;
			boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

			// We have to provide an empty deleter to avoid destroying the global stream object
			boost::shared_ptr<std::ostream> std_cout_stream(&std::cout, boost::null_deleter());
			sink->locked_backend()->add_stream(std_cout_stream);

			// Register the sink in the logging core
			boost::log::core::get()->add_sink(sink);
		}
	}
}