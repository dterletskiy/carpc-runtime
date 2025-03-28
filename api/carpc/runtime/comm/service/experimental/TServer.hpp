#pragma once

#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/comm/service/IServer.hpp"
#include "carpc/runtime/comm/service/experimental/TGenerator.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "TServer"



namespace carpc::service::experimental::__private__ {

   template< typename _TGenerator >
      class TProxy;

   template< typename _TGenerator >
      class TServer;

} // namespace carpc::service::experimental::__private__



namespace carpc::service::experimental::__private_server__ {

   enum class eRequestStatus : std::uint8_t { BUSY, READY };

   struct RequestInfo
   {
      RequestInfo( const comm::sequence::ID _server_seq_id )
         : server_seq_id( _server_seq_id )
      { }
      RequestInfo(
               const comm::sequence::ID _server_seq_id,
               const comm::sequence::ID _client_seq_id,
               const Address _client_addr
            )
         : server_seq_id( _server_seq_id )
         , client_seq_id( _client_seq_id )
         , client_addr( _client_addr )
      { }
      const bool operator<( const RequestInfo& other ) const { return server_seq_id < other.server_seq_id; }

      comm::sequence::ID server_seq_id = comm::sequence::ID::invalid;
      comm::sequence::ID client_seq_id = comm::sequence::ID::invalid;
      const Address client_addr;
   };

   struct RequestStatus
   {
      eRequestStatus status = eRequestStatus::READY;
      comm::sequence::ID processing_server_seq_id = comm::sequence::ID::invalid;
      std::set< RequestInfo > info_set;
   };

   template< typename _TGenerator >
   class MethodProcessor
   {
      using tServer = __private__::TServer< _TGenerator >;
      using tProxy = __private__::TProxy< _TGenerator >;

      public:
         using tRequestStatusMap = std::map< typename _TGenerator::method::tEventID, RequestStatus >;

      public:
         MethodProcessor( tServer* );
         ~MethodProcessor( );

         void process( const typename _TGenerator::method::tEvent& );
         const comm::sequence::ID& unblock_request( );
         void prepare_response( const comm::sequence::ID );
         template< typename tResponseData, typename... Args >
            void response( const Args&... args );

      private:
         bool pre_process( const typename _TGenerator::method::tEvent& );

      private:
         tServer*                                           mp_server = nullptr;
         tRequestStatusMap                                  m_request_status_map;
         comm::sequence::ID                                 m_seq_id = comm::sequence::ID::zero;
         std::optional< typename _TGenerator::method::tEventID >  m_processing_event_id = std::nullopt;
         std::optional< comm::sequence::ID >                m_prepare_response_seq_id = std::nullopt;
   };

   template< typename _TGenerator >
   MethodProcessor< _TGenerator >::MethodProcessor( tServer* p_server )
      : mp_server( p_server )
   {
   }

   template< typename _TGenerator >
   MethodProcessor< _TGenerator >::~MethodProcessor( )
   {
   }

   template< typename _TGenerator >
   bool MethodProcessor< _TGenerator >::pre_process( const typename _TGenerator::method::tEvent& event )
   {
      const typename _TGenerator::method::tEventID event_id = event.info( ).id( );
      const comm::service::ID from_id = event.info( ).from( );
      const comm::service::ID to_id = event.info( ).to( );
      const comm::sequence::ID seq_id = event.info( ).seq_id( );
      const auto from_context = event.context( );

      // @TDA-DEBUG:
      if( to_id != mp_server->id( ) )
      {
         SYS_ERR( "%s != %s", to_id.dbg_name( ).c_str( ), mp_server->id( ).dbg_name( ).c_str( ) );
      }

      auto result = m_request_status_map.emplace( event_id, RequestStatus{ } );
      auto& request_status = result.first->second;

      // Check if received request has connected response.
      // If current request does not have connected response request status should not be set to busy
      // and all data for response is not needed.
      if( false == _TGenerator::T::method::has_response( event_id ) )
         return true;

      // Check request status for current request ID
      if( eRequestStatus::BUSY == request_status.status )
      {
         SYS_WRN( "request busy: %s", event_id.c_str( ) );
         // Sending event with request busy id
         _TGenerator::method::tEvent::create(
            typename _TGenerator::method::tEventUserSignature(
               mp_server->signature( ).role( ),
               event_id,
               carpc::service::eType::BUSY,
               mp_server->id( ),
               from_id,
               seq_id
            )
         )->send( from_context );
         return false;
      }

      // Set request busy for current request ID
      request_status.status = eRequestStatus::BUSY;
      // Increment common sequence ID and set it's value to current processing sequence ID
      // for current request ID
      request_status.processing_server_seq_id = ++m_seq_id;
      // Store RequestInfo structure to request info set 
      const auto emplace_result = request_status.info_set.emplace(
            m_seq_id, seq_id, Address{ from_context, from_id }
         );
      if( false == emplace_result.second )
      {
         SYS_WRN( "emplace error: %s", m_seq_id.dbg_name( ).c_str( ) );
         return false;
      }
      return true;
   }

   template< typename _TGenerator >
   void MethodProcessor< _TGenerator >::process( const typename _TGenerator::method::tEvent& event )
   {
      m_processing_event_id = event.info( ).id( );
      if( true == pre_process( event ) )
         mp_server->process_request_event( event );
      m_processing_event_id.reset( );
   }

   template< typename _TGenerator >
   const comm::sequence::ID& MethodProcessor< _TGenerator >::unblock_request( )
   {
      if( std::nullopt == m_processing_event_id )
      {
         SYS_WRN( "unable unblock request" );
         return comm::sequence::ID::invalid;
      }

      // Find request id in request status map
      auto iterator_status_map = m_request_status_map.find( m_processing_event_id.value( ) );
      if( m_request_status_map.end( ) == iterator_status_map )
      {
         SYS_WRN( "not a request ID: %s", m_processing_event_id.value( ).c_str( ) );
         return comm::sequence::ID::invalid;
      }

      auto& request_status = iterator_status_map->second;
      request_status.status = eRequestStatus::READY;
      return request_status.processing_server_seq_id;
   }

   template< typename _TGenerator >
   void MethodProcessor< _TGenerator >::prepare_response( const comm::sequence::ID seq_id )
   {
      m_prepare_response_seq_id = seq_id;
   }

   template< typename _TGenerator >
   template< typename tResponseData, typename... Args >
   void MethodProcessor< _TGenerator >::response( const Args&... args )
   {
      // Find request id in request status map
      auto iterator_status_map = m_request_status_map.find( tResponseData::ID );
      if( m_request_status_map.end( ) == iterator_status_map )
      {
         SYS_WRN( "not a method ID: %s", tResponseData::ID.c_str( ) );
         return;
      }
      auto& request_status = iterator_status_map->second;
      auto& request_info_set = request_status.info_set;

      // Get searching sequence ID depending on 'm_prepare_response_seq_id' and 'processing_server_seq_id'
      comm::sequence::ID searching_seq_id = m_prepare_response_seq_id.value_or(
            request_status.processing_server_seq_id
         );
      // Search for RequestInfo structure in request info set for current searching sequence ID
      auto iterator_request_info_set = request_info_set.find( RequestInfo{ searching_seq_id } );
      if( request_info_set.end( ) == iterator_request_info_set )
      {
         SYS_WRN( "request info for currnet sequence id has not been found: %s",
               searching_seq_id.dbg_name( ).c_str( )
            );
         return;
      }

      const RequestInfo& request_info = *iterator_request_info_set;

      typename _TGenerator::method::tEventUserSignature event_signature(
         mp_server->signature( ).role( ),
         tResponseData::ID,
         carpc::service::eType::RESPONSE,
         mp_server->id( ),
         request_info.client_addr.id( ),
         request_info.client_seq_id
      );
      typename _TGenerator::method::tEventData event_data( std::make_shared< tResponseData >( args... ) );
      _TGenerator::method::tEvent::create( event_signature )->
         data( event_data )->send( request_info.client_addr.context( ) );

      request_info_set.erase( iterator_request_info_set );
      request_status.status = eRequestStatus::READY;
      m_prepare_response_seq_id.reset( );
   }


} // namespace carpc::service::experimental::__private_server__



namespace carpc::service::experimental::__private_server__ {

   template< typename _TGenerator >
   struct NotificationStatus
   {
      using tServer = __private__::TServer< _TGenerator >;

      public:
         bool is_subscribed( ) const;
         bool add_subscriber( const application::Context&, const comm::service::ID& );
         bool remove_subscriber( const application::Context&, const comm::service::ID& );
         template< typename tAttributeData, typename... Args >
            void notify_subscribers( const tServer&, const Args& ... args );

      public:
         std::shared_ptr< typename _TGenerator::attribute::tBaseData > data( ) const;
      private:
         std::set< Address > subscribers;
         std::shared_ptr< typename _TGenerator::attribute::tBaseData > mp_data = nullptr;
   };

   template< typename _TGenerator >
   std::shared_ptr< typename _TGenerator::attribute::tBaseData >
      NotificationStatus< _TGenerator >::data( ) const
   {
      return mp_data;
   }

   template< typename _TGenerator >
   bool NotificationStatus< _TGenerator >::is_subscribed( ) const
   {
      return !( subscribers.empty( ) );
   }

   template< typename _TGenerator >
   bool NotificationStatus< _TGenerator >::add_subscriber(
         const application::Context& context, const comm::service::ID& service_id
      )
   {
      // In case of current process PID can have two different exact values but what could mean
      // the same logic values.
      // It is current process ID and appliocation::process::local
      // Here we should standartize what exact value should be stored in collection.
      return subscribers.emplace(
                  Address{
                     application::Context{
                        context.tid( ),
                        context.is_external( ) ? context.pid( ) : application::process::local
                     },
                     service_id
                  }
               ).second;
   }

   template< typename _TGenerator >
   bool NotificationStatus< _TGenerator >::remove_subscriber(
         const application::Context& context, const comm::service::ID& service_id
      )
   {
      // In case of current process PID can have two different exact values but what could mean
      // the same logic values.
      // It is current process ID and appliocation::process::local
      // Here we should standartize what exact value should be stored in collection.
      return 0 != subscribers.erase(
                     Address{
                        application::Context{
                           context.tid( ),
                           context.is_external( ) ? context.pid( ) : application::process::local
                        },
                        service_id
                     }
                  );
   }

   template< typename _TGenerator >
   template< typename tAttributeData, typename... Args >
   void NotificationStatus< _TGenerator >::notify_subscribers( const tServer& server, const Args& ... args )
   {
      mp_data = std::make_shared< tAttributeData >( args... );

      if( false == is_subscribed( ) )
      {
         SYS_VRB( "there are no subscribers to notify: %s", tAttributeData::ID.c_str( ) );
         return;
      }


      for( const auto& subscriber : subscribers )
      {
         typename _TGenerator::attribute::tEventUserSignature event_signature(
            server.signature( ).role( ),
            tAttributeData::ID,
            carpc::service::eType::NOTIFICATION,
            server.id( ),
            subscriber.id( )
         );
         typename _TGenerator::attribute::tEventData event_data( mp_data );
         _TGenerator::attribute::tEvent::create( event_signature )->
            data( event_data )->send( subscriber.context( ) );
      }
   }



   template< typename _TGenerator >
   class AttributeProcessor
   {
      using tServer = __private__::TServer< _TGenerator >;
      using tProxy = __private__::TProxy< _TGenerator >;

      using tNotificationStatus = NotificationStatus< _TGenerator >;
      using tAttributeStatusMap = std::map< typename _TGenerator::attribute::tEventID, tNotificationStatus >;

      public:
         AttributeProcessor( tServer* );
         ~AttributeProcessor( );

      public:
         bool process( const typename _TGenerator::attribute::tEvent& );
         template< typename tAttributeData, typename... Args >
            void notify( const Args&... args );
         template< typename tAttributeData >
            const tAttributeData* attribute( ) const;

      private:
         tServer*                mp_server = nullptr;
         tAttributeStatusMap     m_attribute_status_map;
   };

   template< typename _TGenerator >
   AttributeProcessor< _TGenerator >::AttributeProcessor( tServer* p_server )
      : mp_server( p_server )
   {
   }

   template< typename _TGenerator >
   AttributeProcessor< _TGenerator >::~AttributeProcessor( )
   {
   }

   template< typename _TGenerator >
   bool AttributeProcessor< _TGenerator >::process( const typename _TGenerator::attribute::tEvent& event )
   {
      const typename _TGenerator::attribute::tEventID event_id = event.info( ).id( );
      const carpc::service::eType event_type = event.info( ).type( );
      const comm::service::ID from_id = event.info( ).from( );
      const auto from_context = event.context( );

      if( carpc::service::eType::SUBSCRIBE == event_type )
      {
         // Processing attribute subscription event

         auto emplace_result = m_attribute_status_map.emplace( event_id, tNotificationStatus{ } );
         auto attribute_status_map_iterator = emplace_result.first;

         // In case there is cached attribute value => send notification event immidiatly to subscriber
         auto& notification_status = attribute_status_map_iterator->second;
         if( notification_status.add_subscriber( from_context, from_id ) )
         {
            // @TDA-DEBUG:
            SYS_VRB(
               "subscriber '%s' added for attribute '%s'",
               Address{ from_context, from_id }.dbg_name( ).c_str( ),
               event_id.c_str( )
            );
         }
         if( notification_status.data( ) )
         {
            typename _TGenerator::attribute::tEventUserSignature event_signature(
               mp_server->signature( ).role( ),
               event_id,
               carpc::service::eType::NOTIFICATION,
               mp_server->id( ),
               from_id
            );
            typename _TGenerator::attribute::tEventData event_data( notification_status.data( ) );
            _TGenerator::attribute::tEvent::create( event_signature )->
               data( event_data )->send( from_context );
         }
      }
      else if( carpc::service::eType::UNSUBSCRIBE == event_type )
      {
         // Processing attribute unsubscription event

         auto attribute_status_map_iterator = m_attribute_status_map.find( event_id );
         if( m_attribute_status_map.end( ) == attribute_status_map_iterator )
         {
            SYS_WRN( "there are no subscriptions for : %s", event_id.c_str( ) );
            return false;
         }

         auto& notification_status = attribute_status_map_iterator->second;
         if( notification_status.remove_subscriber( from_context, from_id ) )
         {
            // @TDA-DEBUG:
            SYS_VRB(
               "subscriber '%s' removed for attribute '%s'",
               Address{ from_context, from_id }.dbg_name( ).c_str( ),
               event_id.c_str( )
            );
         }
      }
      else
      {
         SYS_WRN( "unexpected event type: %s", event_type.c_str( ) );
         return false;
      }

      return true;
   }

   template< typename _TGenerator >
   template< typename tAttributeData, typename... Args >
   void AttributeProcessor< _TGenerator >::notify( const Args&... args )
   {
      auto attribute_status_map_iterator = m_attribute_status_map.find( tAttributeData::ID );
      if( m_attribute_status_map.end( ) == attribute_status_map_iterator )
      {
         SYS_WRN( "subscribers not found: %s", tAttributeData::ID.c_str( ) );
         return;
      }

      auto& notification_status = attribute_status_map_iterator->second;
      notification_status.template notify_subscribers< tAttributeData >( *mp_server, args... );
   }

   template< typename _TGenerator >
   template< typename tAttributeData >
   const tAttributeData* AttributeProcessor< _TGenerator >::attribute( ) const
   {
      auto attribute_status_map_iterator = m_attribute_status_map.find( tAttributeData::ID );
      if( m_attribute_status_map.end( ) == attribute_status_map_iterator )
      {
         SYS_WRN( "not a notification ID: %s", tAttributeData::ID.c_str( ) );
         return nullptr;
      }

      const auto& notification_status = attribute_status_map_iterator->second;
      return static_cast< tAttributeData* >( notification_status.data( ).get( ) );
   }

} // namespace carpc::service::experimental::__private_server__



namespace carpc::service::experimental::__private__ {

   template< typename _TGenerator >
   class TServer
      : public IServer
      , public _TGenerator::method::tEventConsumer
      , public _TGenerator::attribute::tEventConsumer
   {
      using tServer = TServer< _TGenerator >;
      using tMethodProcessor = __private_server__::MethodProcessor< _TGenerator >;
      using tAttributeProcessor = __private_server__::AttributeProcessor< _TGenerator >;

      friend class __private_server__::MethodProcessor< _TGenerator >;

      public:
         TServer( const carpc::async::tAsyncTypeID&, const std::string&, const bool );
         ~TServer( ) override;

      private:
         void connected( const Address& ) override final;
         void disconnected( const Address& ) override final;
         void connected( ) override = 0;
         void disconnected( ) override = 0;

      protected:
         template< typename tResponseData, typename... Args >
            void response( const Args&... args );
         template< typename tAttributeData, typename... Args >
            void notify( const Args&... args );
         template< typename tAttributeData >
            const tAttributeData* attribute( ) const;
         template< typename tRequestData >
            const tRequestData* get_event_data( const typename _TGenerator::method::tEvent& event );
         const comm::sequence::ID& unblock_request( );
         void prepare_response( const comm::sequence::ID );

      private:
         void process_event( const typename _TGenerator::method::tEvent& ) override final;
         void process_event( const typename _TGenerator::attribute::tEvent& ) override final;
      private:
         virtual void process_request_event( const typename _TGenerator::method::tEvent& ) = 0;

      private:
         tMethodProcessor     m_method_processor;
         tAttributeProcessor  m_attribute_processor;
   };



   template< typename _TGenerator >
   TServer< _TGenerator >::TServer(
            const carpc::async::tAsyncTypeID& interface_type_id,
            const std::string& role_name,
            const bool is_export
         )
      : IServer( interface_type_id, role_name, is_export )
      , _TGenerator::method::tEventConsumer( )
      , _TGenerator::attribute::tEventConsumer( )
      , m_method_processor( this )
      , m_attribute_processor( this )
   {
      for( auto method_event_id : typename _TGenerator::method::tEventID( ) )
      {
         _TGenerator::method::tEvent::set_notification(
            this,
            typename _TGenerator::method::tEventUserSignature(
               signature( ).role( ),
               method_event_id,
               carpc::service::eType::REQUEST,
               comm::service::ID::invalid,
               id( )
            )
         );
      }
      for( auto attribute_event_id : typename _TGenerator::attribute::tEventID( ) )
      {
         _TGenerator::attribute::tEvent::set_notification(
            this,
            typename _TGenerator::attribute::tEventUserSignature(
               signature( ).role( ),
               attribute_event_id,
               carpc::service::eType::SUBSCRIBE,
               comm::service::ID::invalid,
               id( )
            )
         );

         _TGenerator::attribute::tEvent::set_notification(
            this,
            typename _TGenerator::attribute::tEventUserSignature(
               signature( ).role( ),
               attribute_event_id,
               carpc::service::eType::UNSUBSCRIBE,
               comm::service::ID::invalid,
               id( )
            )
         );
      }
   }

   template< typename _TGenerator >
   TServer< _TGenerator >::~TServer( )
   {
      _TGenerator::method::tEvent::clear_all_notifications( this );
      _TGenerator::attribute::tEvent::clear_all_notifications( this );
   }

   template< typename _TGenerator >
   void TServer< _TGenerator >::connected( const Address& proxy_address )
   {
      connected( );
   }

   template< typename _TGenerator >
   void TServer< _TGenerator >::disconnected( const Address& proxy_address )
   {
      disconnected( );
   }

   template< typename _TGenerator >
   void TServer< _TGenerator >::process_event( const typename _TGenerator::method::tEvent& event )
   {
      SYS_VRB( "processing event: %s", event.info( ).dbg_name( ).c_str( ) );
      m_method_processor.process( event );
   }

   template< typename _TGenerator >
   void TServer< _TGenerator >::process_event( const typename _TGenerator::attribute::tEvent& event )
   {
      SYS_VRB( "processing event: %s", event.info( ).dbg_name( ).c_str( ) );
      m_attribute_processor.process( event );
   }

   template< typename _TGenerator >
   const comm::sequence::ID& TServer< _TGenerator >::unblock_request( )
   {
      return m_method_processor.unblock_request( );
   }

   template< typename _TGenerator >
   void TServer< _TGenerator >::prepare_response( const comm::sequence::ID seq_id )
   {
      return m_method_processor.prepare_response( seq_id );
   }

   template< typename _TGenerator >
   template< typename tResponseData, typename... Args >
   void TServer< _TGenerator >::response( const Args&... args )
   {
      m_method_processor.template response< tResponseData >( args... );
   }

   template< typename _TGenerator >
   template< typename tAttributeData, typename... Args >
   void TServer< _TGenerator >::notify( const Args&... args )
   {
      m_attribute_processor.template notify< tAttributeData >( args... );
   }

   template< typename _TGenerator >
   template< typename tAttributeData >
   const tAttributeData* TServer< _TGenerator >::attribute( ) const
   {
      return m_attribute_processor.template attribute< tAttributeData >( );
   }

   template< typename _TGenerator >
   template< typename tRequestData >
   const tRequestData* TServer< _TGenerator >::get_event_data(
         const typename _TGenerator::method::tEvent& event
      )
   {
      if( const tRequestData* p_data = static_cast< tRequestData* >( event.data( )->ptr.get( ) ) )
         return p_data;

      SYS_ERR( "missing data for method ID: %s", event.info( ).id( ).c_str( ) );
      return nullptr;
   }

} // namespace carpc::service::experimental::__private__



namespace carpc::service::experimental {

   template< typename _ServiceTypes >
   class TServer : public __private__::TServer< TGenerator< _ServiceTypes > >
   {
      public:
         TServer( const std::string&, const bool );

         using tMethod = typename TGenerator< _ServiceTypes >::Method;
         using tAttribute = typename TGenerator< _ServiceTypes >::Attribute;
   };

   template< typename _ServiceTypes >
   TServer< _ServiceTypes >::TServer( const std::string& role_name, const bool is_export )
      : __private__::TServer< TGenerator< _ServiceTypes > >(
            TGenerator< _ServiceTypes >::interface_type_id, role_name, is_export
         )
   {
      REGISTER_EVENT( tMethod );
      REGISTER_EVENT( tAttribute );
   }

} // namespace carpc::service::experimental



#undef CLASS_ABBR
