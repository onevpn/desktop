#pragma once

namespace PubSub { namespace Logging {

namespace Details {

	template<class T, T t> class Test { };

	template<class T> class LogLevelHelper
	{
		struct Mix { int logLevel(); };

		struct Base : T , Mix { };

		template<class X> static int helper(Test< int (Mix::*)() , &X::logLevel> * )
			{ return 25; }
		template<class X> static int helper(...)
			{ return T::logLevel(); }

	public:
		static int maybeCallIt()
			{ return helper<Base>(NULL); }
	};

	template<class T> class IsSignificantChangeHelper
	{
		struct Mix { bool isSignificantChange(T const *, T const *); };

		struct Base : T , Mix { };

		template<class X> static bool helper(T const *before, T const *after
		, Test< bool (Mix::*)(T const *, T const *) , &X::isSignificantChange> * )
		{ return true; }

		template<class X> static bool helper(T const *before, T const *after, ...)
		{ return T::isSignificantChange(before, after); }

	public:
		static bool maybeCallIt(T const *before, T const *after)
			{ return helper<Base>(before, after, NULL); }
	};

} // namespace Details

template<class T> int logLevel()
	{ return Details::LogLevelHelper<T>::maybeCallIt(); }

template<class T> bool isSignificantChange(T const *before, T const *after)
	{ return Details::IsSignificantChangeHelper<T>::maybeCallIt(before, after); }

// log insignificant changes with greater log level
template<class T> int logLevelForChange(T const *before, T const *after)
	{ return logLevel<T>() + (isSignificantChange(before, after) ? 0 : 10); }

} }
