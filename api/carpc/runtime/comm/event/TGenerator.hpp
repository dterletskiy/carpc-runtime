#pragma once

#include <type_traits>
#include <concepts>

#include "carpc/runtime/comm/event/ISignature.hpp"
#include "carpc/runtime/comm/event/IData.hpp"
#include "carpc/runtime/comm/event/TEvent.hpp"
#include "carpc/runtime/comm/event/TConsumer.hpp"



namespace carpc::runtime::comm::event {

   template< typename T >
   concept EventDataType = std::is_base_of_v< IData, T >;

   template< typename T >
   concept EventSignatureType = std::is_base_of_v< ISignature, T >;

   template<
         typename _ServiceType,
         typename _EventType,
         typename _DataType,
         typename _SignatureType
      >
      // requires EventDataType< _DataType > &&
      //          EventSignatureType< _SignatureType >
   class TGenerator
   {

      static_assert( std::is_base_of_v< IData, _DataType >,
         "TGenerator requires Data to inherit from IData" );

      static_assert( std::is_base_of_v< ISignature, _SignatureType >,
         "TGenerator requires Signature to inherit from ISignature" );

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
