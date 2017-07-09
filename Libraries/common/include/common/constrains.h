#pragma once

struct noncopyable
{
	noncopyable(const noncopyable&&) = delete;
	noncopyable(const noncopyable&) = delete;
	
	noncopyable() = default;
};