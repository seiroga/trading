#pragma once

#include <common/constrains.h>

#include <string>

namespace test_helpers
{
	struct base_fixture
	{
		struct temp_folder
		{
			const std::wstring path;

		public:
			static std::wstring get_default_path();

		public:
			temp_folder(const std::wstring& path = get_default_path() + L"\\" + unique_string());
			~temp_folder();
		};

		static std::wstring unique_string();
	};

	struct temp_dir_fixture : base_fixture
	{
		temp_folder data_root;

		temp_dir_fixture()
			: data_root(temp_folder::get_default_path())
		{
		}
	};
}