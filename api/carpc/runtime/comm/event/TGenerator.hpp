#pragma once

#include "carpc/runtime/comm/event/TEvent.hpp"
#include "carpc/runtime/comm/event/TConsumer.hpp"



namespace carpc::runtime::comm::event {

   template<
         typename _ServiceType,
         typename _EventType,
         typename _DataType,
         typename _SignatureType
      >
   class TGenerator
   {
      using tGenerator = TGenerator<
         _ServiceType, _EventType, _DataType, _SignatureType >;

      public:
         struct Config
         {
            using tEvent         = TEvent< tGenerator >;
            using tSignature     = _SignatureType;
            using tConsumer      = TConsumer< tGenerator >;
            using tData          = _DataType;
            using tService       = _ServiceType;
            using tProcessor     = void ( tConsumer::* )( const tEvent& );
         };
   };

} // namespace carpc::runtime::comm::event
