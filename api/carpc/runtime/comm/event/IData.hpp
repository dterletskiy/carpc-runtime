#pragma once

#include "carpc/base/memory/RefCounted.hpp"



namespace carpc::runtime::comm::event {

   class IData : public carpc::memory::RefCounted
   {
      public:
         IData( ) = default;
         virtual ~IData( ) = default;

         virtual void clone_from( const IData& other ) { }\
   };

} // namespace carpc::runtime::comm::event
