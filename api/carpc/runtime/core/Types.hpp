#pragma once

#include "carpc/base/types/ID.hpp"
#include "carpc/base/types/Name.hpp"



namespace carpc::runtime::core {

   namespace process
   {
      class Process;
      using ID = carpc::TID< Process, uint32_t >;
      const ID invalid = ID::invalid;
      const ID broadcast = ID::invalid - ID::VALUE_TYPE( 1 );
      const ID& current_id( );
      using Name = carpc::TName< Process >;
   }

   namespace thread
   {
      class Thread;
      using ID = carpc::TID< Thread, uint64_t >;
      const ID invalid = ID::invalid;
      const ID broadcast = ID::invalid - ID::VALUE_TYPE( 1 );
      const ID& current_id( );
      using Name = carpc::TName< Thread >;
   }

} // namespace carpc::runtime::core
