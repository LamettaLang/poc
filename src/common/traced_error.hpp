#pragma once

#include <exception>
#include <stacktrace>

struct stacktraced_error
{
	std::exception mError;
	std::stacktrace mTrace;

	stacktraced_error(std::exception&& exception, std::stacktrace stack = std::stacktrace::current())
	: mError(std::move(exception)), mTrace(stack)
	{}

	stacktraced_error(std::exception const& exception, std::stacktrace stack = std::stacktrace::current())
	: mError(exception), mTrace(stack)
	{}

	inline auto
	what() const -> char const*
	{
		return mError.what();
	}

	inline auto
	stacktrace() const -> std::stacktrace const&
	{
		return mTrace;
	}
};
