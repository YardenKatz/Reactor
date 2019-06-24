#ifndef __REACTOR_HPP__
#define __REACTOR_HPP__

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <map>
#include "cpp98_utils.hpp"


namespace ilrd
{
class Finalizer;
typedef boost::function<void(int)> func_type;

class Reactor: private boost::noncopyable
{
public:
	enum Usage
	{
		EXCEPT,
		READ,
		WRITE,
		USAGE_COUNT
	};

	enum Status // Run() return values
	{
		STOP_CALLED,
		EMPTY,
		ERROR,
		STATUS_COUNT
	};

	static const int limit = 1024;

	explicit Reactor();

	// ~Reactor() noexcept = generated;

	// - Adding the same fd with the same Usage more than once is not allowed.
	// An "ExceptionRepeatingFD" exception will be thrown.
	// - Adding a non-valid fd is not allowed.
	// An "ExceptionNonValidFD" exception will be thrown.
	// - If Adding fd overflows limit of fd's,
	// "ExceptionFDsOverflow" exception will be thrown.
	void Add(int fd, Usage usage, func_type callback_func);

	// Removing a non-existing fd is not allowed.
	// An "ExceptionRemovingNonExistingFD" exception will be thwrown.
	void Remove(int fd, Usage usage);

	// function in blocking mode untill Stop() is called
	// by a callback function, or until reactor is empty of fd's
	Status Run(); 

	// NON-REENTRANT!! must be called by callback functio
	// (not by other thread/process)	  
	void Stop();

	class ExceptionReactorLogic: public std::logic_error
	{
	public:
		explicit ExceptionReactorLogic(const char *str =
								  	   "ilrd::Reactor::ExceptionReactorLogic");
	};

	class ExceptionRepeatingFD: public ExceptionReactorLogic
	{
	public:
		explicit ExceptionRepeatingFD(const char *str =
									  "ilrd::Reactor::ExceptionRepeatingFD");
	};

	class ExceptionNonValidFD: public ExceptionReactorLogic
	{
	public:
		explicit ExceptionNonValidFD(const char *str =
									 "ilrd::Reactor::ExceptionNonValidFD");
	};

	class ExceptionRemovingNonExistingFD: public ExceptionReactorLogic
	{
	public:
		explicit ExceptionRemovingNonExistingFD(const char *str =
				 "ilrd::Reactor::ExceptionRemovingNonExistingFD");
	};

	class ExceptionReactorRuntime: public std::runtime_error
	{
	public:
		explicit ExceptionReactorRuntime(const char *str =
				 "ilrd::Reactor::ExceptionReactorRuntime");
	};

	class ExceptionFDsOverflow: public ExceptionReactorRuntime
	{
	// enum Usage
	// {
	// 	EXCEPT,
	// 	READ,
	// 	WRITE,
	// 	USAGE_COUNT
	// };
	public:
		explicit ExceptionFDsOverflow(const char *str =
									  "ExceptionFDsOverflow");
	};

	class ExceptionSelectBadAlloc: public std::runtime_error
	{
	public:
		explicit ExceptionSelectBadAlloc(const char *str = 
										"ExceptionSelectBadALloc");
	};
	 
private:
	typedef std::map<int, func_type> map_type;
	
	friend class Finalizer;

	volatile bool m_isRunning;
	bool m_reactHappened;
	map_type reactions_tables[3];

	class FDSetInitIMP
	{
	// enum Usage
	// {
	// 	EXCEPT,
	// 	READ,
	// 	WRITE,
	// 	USAGE_COUNT
	// };
	public:
		explicit FDSetInitIMP(fd_set& set_);
		// ~FDSetInitIMP() noexcept = generated

		void operator()(const std::pair<int, func_type>& file_des);

	private:
		fd_set& m_set;
	};

	class FDTestAndRunIMP
	{
	public:
		explicit FDTestAndRunIMP(fd_set& set_);
		// ~FDTestAndRunIMP() noexcept = generated

		bool operator()(std::pair<int, func_type> file_des);

	private:
		fd_set& m_set;
	}; 
	
};

class Finalizer
{
public:
    Finalizer(Reactor& r);
    ~Finalizer() noexcept;

private:
    Reactor& m_r;
};

} //ilrd


#endif //__REACTOR_HPP__
