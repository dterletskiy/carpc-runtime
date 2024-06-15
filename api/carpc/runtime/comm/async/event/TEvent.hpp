#pragma once

#include "carpc/runtime/comm/async/event/IEvent.hpp"

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
         TEvent( ) = default;
         TEvent( const tUserSignature& signature )
         {
            mp_signature = tSignature::create( signature );
         }
      public:
         ~TEvent( ) override = default;

      // static functions
      public:
         static const bool set_notification( tConsumer* p_consumer, const tUserSignature& signature )
         {
            return IEvent::set_notification( p_consumer, tSignature::create( signature ) );
         }
         template< typename ... TYPES >
         static const bool set_notification( tConsumer* p_consumer, const TYPES& ... signature_parameters )
         {
            return IEvent::set_notification(
                  p_consumer, tSignature::create( tUserSignature{ signature_parameters... } )
               );
         }
         static const bool clear_notification( tConsumer* p_consumer, const tUserSignature& signature )
         {
            return IEvent::clear_notification( p_consumer, tSignature::create( signature ) );
         }
         template< typename ... TYPES >
         static const bool clear_notification( tConsumer* p_consumer, const TYPES& ... signature_parameters )
         {
            return IEvent::clear_notification(
                  p_consumer, tSignature::create( tUserSignature{ signature_parameters... } )
               );
         }
         static const bool clear_all_notifications( tConsumer* p_consumer )
         {
            return IEvent::clear_all_notifications( p_consumer, tSignature::create( ) );
         }

      public:
         // This "create" static function is used only for creating concrete event type with all empty data
         // what should be filled during serialization.
         static IEvent::tSptr create_empty( )
         {
            return std::shared_ptr< tEvent >( new tEvent( tUserSignature( ) ) );
         }
         // Create event with user defined signature passed as argument
         static std::shared_ptr< tEvent > create( const tUserSignature& signature )
         {
            return std::shared_ptr< tEvent >( new tEvent( signature ) );
         }
         // Create event with user defined signature built using variadic arguments
         // In case if arguments list is empty, default user signature constructor will be used
         template< typename ... TYPES >
         static std::shared_ptr< tEvent > create( const TYPES& ... signature_parameters )
         {
            return std::shared_ptr< tEvent >( new tEvent( tUserSignature{ signature_parameters... } ) );
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
         const tUserSignature& info( ) const { return mp_signature->user_signature( ); }
         const IAsync::ISignature::tSptr signature( ) const override { return mp_signature; }
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
         tPriority m_priority = carpc::priority( ePriority::DEFAULT );
   };

} // namespace carpc::async



#undef CLASS_ABBR
