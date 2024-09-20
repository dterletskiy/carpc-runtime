#pragma once

#include "carpc/base/helpers/macros/types.hpp"
#include "carpc/runtime/comm/async/event/IEvent.hpp"
#include "carpc/runtime/comm/async/event/TSignature.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "TEvent"



namespace carpc::async {

   template< typename _Generator >
   class TEvent : public IEvent
   {
      // using and types
      public:
         using tEvent         = typename _Generator::Config::tEvent;
         using tEventPtr      = typename std::shared_ptr< tEvent >;
         using tConsumer      = typename _Generator::Config::tConsumer;
         using tService       = typename _Generator::Config::tService;
         using tData          = typename _Generator::Config::tData;
         using tDataPtr       = typename std::shared_ptr< tData >;
         using tSignature     = typename _Generator::Config::tSignature;
         using tUserSignature = typename _Generator::Config::tUserSignature;

      // constructors
      protected:
         TEvent( )
         {
            mp_signature = tSignature::create( );
         }
         TEvent( const tUserSignature& user_signature )
         {
            mp_signature = tSignature::create( user_signature );
         }
      public:
         ~TEvent( ) override = default;

      // These static functions are intended to be instantiated for user-defined events
      // WITHOUT a user-specified signature.
      // In this case, subscribing to and unsubscribing from such events occurs WITHOUT
      // specifying a signature.
      // These functions are templated solely to enable the implementation of the SFINAE mechanism.
      public:
         template< typename U = tUserSignature >
         static const typename std::enable_if_t< std::is_same_v< U, typename simple::Signature >, bool >
            set_notification( tConsumer* p_consumer )
            {
               return IEvent::set_notification( p_consumer, tSignature::create( ) );
            }

         template< typename U = tUserSignature >
         static const typename std::enable_if_t< std::is_same_v< U, typename simple::Signature >, bool >
            clear_notification( tConsumer* p_consumer )
            {
               return IEvent::clear_notification( p_consumer, tSignature::create( ) );
            }

         template< typename U = tUserSignature >
         static typename std::enable_if_t< std::is_same_v< U, typename simple::Signature >, tEventPtr >
            create( )
            {
               return tEventPtr( new tEvent( ) );
            }

      // These static functions are intended to be instantiated for user-defined events
      // WITH a user-specified signature.
      // In this case, subscribing to and unsubscribing from such events occurs WITH
      // specifying a signature or parameters pack for building this signature.
      // These functions are templated solely to enable the implementation of the SFINAE mechanism.
      public:
         template< typename U = tUserSignature >
         static const typename std::enable_if_t< not std::is_same_v< U, typename simple::Signature >, bool >
            set_notification( tConsumer* p_consumer, const tUserSignature& user_signature )
            {
               return IEvent::set_notification( p_consumer, tSignature::create( user_signature ) );
            }

         template< typename ... TYPES, typename U = tUserSignature >
         static const typename std::enable_if_t< ( not std::is_same_v< U, typename simple::Signature > ) and 0 != sizeof...( TYPES ), bool >
            set_notification( tConsumer* p_consumer, const TYPES& ... signature_parameters )
            {
               return IEvent::set_notification(
                     p_consumer, tSignature::create( tUserSignature{ signature_parameters... } )
                  );
            }

         template< typename U = tUserSignature >
         static const typename std::enable_if_t< not std::is_same_v< U, typename simple::Signature >, bool >
            clear_notification( tConsumer* p_consumer, const tUserSignature& user_signature )
            {
               return IEvent::clear_notification( p_consumer, tSignature::create( user_signature ) );
            }

         template< typename ... TYPES, typename U = tUserSignature >
         static const typename std::enable_if_t< ( not std::is_same_v< U, typename simple::Signature > ) and 0 != sizeof...( TYPES ), bool >
            clear_notification( tConsumer* p_consumer, const TYPES& ... signature_parameters )
            {
               return IEvent::clear_notification(
                     p_consumer, tSignature::create( tUserSignature{ signature_parameters... } )
                  );
            }

         template< typename U = tUserSignature >
         static const typename std::enable_if_t< not std::is_same_v< U, typename simple::Signature >, tEventPtr >
            create( const tUserSignature& user_signature )
            {
               return tEventPtr( new tEvent( user_signature ) );
            }

         template< typename ... TYPES, typename U = tUserSignature >
         static const typename std::enable_if_t< ( not std::is_same_v< U, typename simple::Signature > ) and 0 != sizeof...( TYPES ), tEventPtr >
            create( const TYPES& ... signature_parameters )
            {
               return tEventPtr( new tEvent( tUserSignature{ signature_parameters... } ) );
            }

      public:
         static const bool clear_all_notifications( tConsumer* p_consumer )
         {
            return IEvent::clear_all_notifications( p_consumer, tSignature::create( ) );
         }

      public:
         // This "create" static function is used only for creating concrete event type with all empty data
         // what should be filled during serialization.
         static IEvent::tSptr create_empty( )
         {
            return std::shared_ptr< tEvent >( new tEvent( ) );
         }

      // virual function
      public:
         void process_event( IAsync::IConsumer* p_consumer ) const override
         {
            static_cast< tConsumer* >( p_consumer )->process_event( *this );
         }

      // serialization / deserialization
      public:
         const bool to_stream_t( ipc::tStream& stream ) const override final
         {
            if constexpr( CARPC_IS_IPC_TYPE( tService ) )
            {
               return ipc::serialize( stream, mp_signature, m_context, m_priority, mp_data );
            }

            return false;
         }
         const bool from_stream_t( ipc::tStream& stream ) override final
         {
            if constexpr( CARPC_IS_IPC_TYPE( tService ) )
            {
               return ipc::deserialize( stream, mp_signature, m_context, m_priority, mp_data );
            }

            return false;
         }

      public:
         const bool is_ipc( ) const override { return CARPC_IS_IPC_TYPE( tService ); }

      // signature
      public:
         template< typename U = tUserSignature >
         const typename std::enable_if_t< not std::is_same_v< U, typename simple::Signature >, tUserSignature >&
            info( ) const
            {
               return mp_signature->user_signature( );
            }
         const IAsync::ISignature::tSptr signature( ) const override
         {
            return mp_signature;
         }
      private:
         typename tSignature::tSptr mp_signature = nullptr;

      // data
      public:
         const tDataPtr data( ) const
         {
            return mp_data;
         }
         tEventPtr data( const tData& data )
         {
            mp_data = std::make_shared< tData >( data );
            return std::shared_ptr< tEvent >( shared_from_this( ), this );
         }
         tEventPtr data( const tDataPtr data )
         {
            mp_data = data;
            return std::shared_ptr< tEvent >( shared_from_this( ), this );
         }
      private:
         tDataPtr mp_data = nullptr;

      // context
      public:
         const application::Context& context( ) const override
         {
            return m_context;
         }
      private:
         application::Context m_context = application::Context::current( );

      // priority
      public:
         const tPriority priority( ) const override
         {
            return m_priority;
         }
         tEventPtr priority( const tPriority& value )
         {
            m_priority = value;
            return std::shared_ptr< tEvent >( shared_from_this( ), this );
         }
      protected:
         tPriority m_priority = priority::DEFAULT;
   };

} // namespace carpc::async



#undef CLASS_ABBR
