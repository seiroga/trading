#pragma once

namespace sb
{
	struct noncopyable
	{
		noncopyable(const noncopyable&&) = delete;
		noncopyable(const noncopyable&) = delete;

		noncopyable() = default;
	};
}