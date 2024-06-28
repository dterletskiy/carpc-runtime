#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/comm/async/callable/ICallable.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "ICallable"


using namespace carpc::async;



const bool ICallable::send( const application::Context& to_context )
{
   return dispatch( to_context );
}

void ICallable::process( IAsync::IConsumer* p_consumer ) const
{
   call( );
}
