#pragma once

#include "carpc/base/memory/RefCounted.hpp"



namespace carpc::runtime::comm::event {

   class ISignature : public carpc::memory::RefCounted
   {
      public:
         ISignature( ) = default;
         virtual ~ISignature( ) = default;

         virtual bool operator<( const ISignature& other ) const = 0;
   };

} // namespace carpc::runtime::comm::event
