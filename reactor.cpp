#include <unistd.h>             /* fcntl */
#include <fcntl.h>              /* fcntl */
#include <errno.h>              /* errno */

#include "reactor.hpp"

namespace ilrd
{

static void SelectFuncIMP(fd_set *sets, int limit);

Reactor::Reactor()
 :m_isRunning(false)
{}

void Reactor::Add(int fd, Usage usage, func_type callback_func)
{  

    // if fd is invalid throw ExceptionNonValidFD
    if ((0 > fd ) || ((limit <= fd) || (-1 == fcntl(fd, F_GETFL))))
    {
        throw ExceptionNonValidFD();
    }

    // if fd_set is full throw ExceptionFDsOverflow
    if(reactions_tables[usage].size() == limit)
    {
        throw ExceptionFDsOverflow();
    }

    // if fd already exists throw ExceptionRepeatingFD
    // insert callback_func to usage map at fd
    if (false == reactions_tables[usage].insert
                                (std::make_pair(fd, callback_func)).second)
    {
        throw ExceptionRepeatingFD();
    }         
}


void Reactor::Remove(int fd, Usage usage)
{
    // if fd doesn't exist throw ExceptionRemovingNonExistingFD;
    if (reactions_tables[usage].find(fd) == reactions_tables[usage].end())
    {
        throw ExceptionRemovingNonExistingFD();
    }

    // remove fd from usage map
    reactions_tables[usage].erase(fd);
}

Reactor::Status Reactor::Run()
{
    // whole function runs while m_isRunning 
    m_isRunning = true;
    int i = 0;
    Finalizer f(*this);

    while (m_isRunning)
    {
        // if all maps are empty return
        int emptyMaps = 0;

        for (i = 0; i < USAGE_COUNT; ++i)
        {
            if (0 == reactions_tables[i].size())
            {
                ++emptyMaps;
            }
        }

        if (USAGE_COUNT == emptyMaps)
        {
            return EMPTY;
        }

        // create 3 fd_sets
        // on every map set corresponding fd in fd_set (with functor)
        fd_set fd_status[USAGE_COUNT];
        for (i = 0; i < USAGE_COUNT; ++i)
        {
            FD_ZERO(&fd_status[i]);
            for_each(reactions_tables[i].begin(), reactions_tables[i].end(), 
                    FDSetInitIMP(fd_status[i]));
        }

        // select (fd_sets) 
        // if returned error - on case of interrupt return to select, 
        // otherwise throw exception
		SelectFuncIMP(fd_status, limit);

        // on every map check if corresponding fd changed in relevant fd_set
        // if so run appropriate function from map on fd (with functor)
        // continue to next iteration after first reaction execution
        for (i = 0; (i < USAGE_COUNT); ++i)
        {
            if(reactions_tables[i].end() != find_if(reactions_tables[i].begin(), 
            reactions_tables[i].end(), FDTestAndRunIMP(fd_status[i])))
            {
                break;
            }
        }
    }

    // Finalizer Turns the lights off
    return STOP_CALLED;
}

void Reactor::Stop()
{
   assert(m_isRunning);

    m_isRunning = false;
}

static void SelectFuncIMP(fd_set *sets, int limit)
{
    if (-1 == select(limit, &sets[Reactor::READ], &sets[Reactor::WRITE], 
                        &sets[Reactor::EXCEPT], NULL))
    {
        switch (errno)
        {
            case (EINTR):
                SelectFuncIMP(sets, limit);
                break;
        
            case (EAGAIN):
                throw std::runtime_error("memory allocation failed");
                break;
            
            case (EBADF):
                throw std::logic_error("invalid file descriptor");
                break;
        }
    }    
}

/*******************************    Finalizer   ******************************/



Finalizer::Finalizer(Reactor& r) 
 : m_r(r)
{}

Finalizer::~Finalizer() noexcept
{
    m_r.m_isRunning = false;
}

/******************************    FDSetInitIMP   ****************************/


Reactor::FDSetInitIMP::FDSetInitIMP(fd_set& set_)
 : m_set(set_)
 {}

void Reactor::FDSetInitIMP::operator()(const std::pair<int, func_type>& file_des)
{
    FD_SET(file_des.first, &m_set);
}


/****************************   FDTestAndRunIMP   *****************************/



Reactor::FDTestAndRunIMP::FDTestAndRunIMP(fd_set& set_)
 : m_set(set_)
 {}

bool Reactor::FDTestAndRunIMP::operator()(std::pair<int, func_type> file_des)
{
    // check if file descriptor is set
    // if set - run callback on file descriptor
    // return if ran callback
  
    if (FD_ISSET(file_des.first, &m_set))
    {
        file_des.second(file_des.first);     
        return true;
    }

    return false;
} 

/************************   exceptions   **********************************/

Reactor::ExceptionReactorLogic::ExceptionReactorLogic(const char *str)
:logic_error(str)
{}

Reactor::ExceptionRepeatingFD::ExceptionRepeatingFD(const char *str)
:ExceptionReactorLogic(str)
{}

Reactor::ExceptionNonValidFD::ExceptionNonValidFD(const char *str)
:ExceptionReactorLogic(str)
{}

Reactor::ExceptionRemovingNonExistingFD::ExceptionRemovingNonExistingFD(const char *str)
:ExceptionReactorLogic(str)
{}

Reactor::ExceptionReactorRuntime::ExceptionReactorRuntime(const char *str)
:runtime_error(str)
{}

Reactor::ExceptionFDsOverflow::ExceptionFDsOverflow(const char *str)
:ExceptionReactorRuntime(str)
{}

} //ilrd
