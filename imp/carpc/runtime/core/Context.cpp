#include "carpc/runtime/core/Context.hpp"



using namespace carpc::runtime::core;



const Context Context::invalid{ };



Context::Context( const process::ID& pid, const thread::ID& tid )
   : m_pid( pid )
   , m_tid( tid )
{
}

const Context& Context::current( )
{
   static const thread_local Context ctx{
         process::current_id( ),
         thread::current_id( )
      };
   return ctx;
}

const Context& Context::internal_local( )
{
   return current( );
}

const Context& Context::internal_broadcast( )
{
   static const Context ctx{
         process::current_id( ),
         thread::broadcast
      };
   return ctx;
}

bool Context::is_internal( ) const
{
   return process::current_id( ) == m_pid;
}

bool Context::is_internal_local( ) const
{
   if( false == is_internal( ) )
      return false;

   return thread::current_id( ) == m_tid;
}

bool Context::is_internal_broadcast( ) const
{
   return is_internal( ) && !is_internal_local( );
}

bool Context::is_external( ) const
{
   return !is_internal( );
}

bool Context::is_valid( ) const
{
   return m_pid.is_valid( ) && m_tid.is_valid( );
}

bool Context::operator==( const Context& other ) const
{
   return ( m_pid == other.m_pid ) && ( m_tid == other.m_tid );
}

bool Context::operator!=( const Context& other ) const
{
   return !( *this == other );
}

bool Context::operator<( const Context& other ) const
{
   if( m_pid != other.m_pid )
      return m_pid < other.m_pid;

   return m_tid < other.m_tid;
}
