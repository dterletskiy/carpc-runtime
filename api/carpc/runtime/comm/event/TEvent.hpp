#pragma once

#include "carpc/runtime/comm/event/IEvent.hpp"

#include "carpc/trace/trace.h"



namespace carpc::runtime::comm::event {

   template< typename _Generator >
   class TEvent : public IEvent
   {
      public:
         using tEvent         = typename _Generator::Config::tEvent;
         using tEventPtr      = typename std::shared_ptr< tEvent >;
         using tConsumer      = typename _Generator::Config::tConsumer;
         using tService       = typename _Generator::Config::tService;
         using tData          = typename _Generator::Config::tData;
         using tDataPtr       = typename std::shared_ptr< tData >;
         using tSignature     = typename _Generator::Config::tSignature;
         using tSignaturePtr  = typename std::shared_ptr< tSignature >;

      public:
         TEvent( ) = default;
         ~TEvent( ) override = default;

         template< typename... Args >
            requires std::constructible_from< tSignature, Args... >
         static tEventPtr create( Args&&... args )
         {
            return std::make_shared< tEvent >( )->signature.set(
                  std::forward< Args >( args )...
               );
         }

         static bool set_all_notifications( tConsumer* p_consumer )
         {
            return true;
         }

         template< typename... Args >
            requires std::constructible_from< tSignature, Args... >
         static bool set_notifications( tConsumer* p_consumer, Args&&... args )
         {
            return true;
         }

         static bool clear_all_notifications( tConsumer* p_consumer )
         {
            return true;
         }

         template< typename... Args >
            requires std::constructible_from< tSignature, Args... >
         static bool clear_notifications( tConsumer* p_consumer, Args&&... args )
         {
            return true;
         }

         void process( IConsumer* p_consumer ) const override
         {
            static_cast< tConsumer* >( p_consumer )->process_event( *this );
         }

      public:
         static const tTypeID& build_type_id( )
         {
            static const tTypeID value{ tTypeID::generate< tEvent >( ) };
            return value;
         }

         const tTypeID& type_id( ) const
         {
            return build_type_id( );
         }

      private:
         template< typename OWNER, typename PROPERTY_PTR, PROPERTY_PTR OWNER::* Member >
         struct PropertyProxy
         {
            OWNER& owner;
            using PROPERTY = typename PROPERTY_PTR::element_type;

            PropertyProxy( OWNER& p )
               : owner( p )
            {
            }

            PROPERTY_PTR get( ) const
            {
               return owner.*Member;
            }

            template< typename... Args >
               requires std::constructible_from< PROPERTY, Args... >
            std::shared_ptr< OWNER > set( Args&&... args )
            {
               owner.*Member = std::make_shared< PROPERTY >(
                  std::forward< Args >( args )... );
               return std::shared_ptr< OWNER >(
                  owner.shared_from_this( ), &owner );

            }
         };

      private:
         tSignaturePtr  mp_signature{ nullptr };
         tDataPtr       mp_data{ nullptr };
      public:
         PropertyProxy< tEvent, tDataPtr, &tEvent::mp_data > data{ *this };
         PropertyProxy< tEvent, tSignaturePtr, &tEvent::mp_signature > signature{ *this };
   };

} // namespace carpc::runtime::comm::event
