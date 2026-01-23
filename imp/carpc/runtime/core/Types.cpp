#include "carpc/osw/osw.h"
#include "carpc/runtime/core/Types.hpp"


namespace carpc::runtime::core {

   namespace process {

      const ID& current_id( )
      {
         static ID value{ carpc::osw::process_id( ) };
         return value;
      }

   }

   namespace thread {

      const ID& current_id( )
      {
         static thread_local ID value{ carpc::osw::thread_id( ) };
         return value;
      }

   }

} // namespace carpc::runtime::core
