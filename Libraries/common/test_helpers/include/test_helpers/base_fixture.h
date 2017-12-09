#pragma once

#include <common/constrains.h>

#include <string>

namespace test_helpers
{
	class base_fixture
	{
		const std::wstring m_data_path;

	public:
		struct temp_folder
		{
			const std::wstring path;

		public:
			static std::wstring get_default_path();

		public:
			temp_folder(const std::wstring& path = get_default_path() + L"\\" + unique_string());
			~temp_folder();
		};

	public:
		static std::wstring unique_string();

		std::wstring get_file_path(const std::wstring& file_name);
		static std::wstring read_file_content(const std::wstring& file_path);

	public:
		base_fixture(const std::wstring& data_subfolder = L"");
	};

	struct temp_dir_fixture : base_fixture
	{
		temp_folder data_root;

		temp_dir_fixture(const std::wstring& data_subfolder = L"")
			: base_fixture(data_subfolder)
			, data_root(temp_folder::get_default_path())
		{
		}
	};
}

#define BOOST_ASSERT_EXCEPT(expr, exception) { bool exception_was_thrown = false; try { expr; } catch(const exception&) { exception_was_thrown = true; } BOOST_ASSERT(exception_was_thrown); }