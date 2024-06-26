#pragma once

#include "carpc/runtime/application/Process.hpp"
#include "carpc/runtime/comm/service/IServer.hpp"
#include "carpc/runtime/comm/service/secure/TGenerator.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "TServerSecure"



namespace carpc::service::secure::__private__ {

   template< typename TYPES >
      class TProxy;

   template< typename TYPES >
      class TServer;

} // namespace carpc::service::secure::__private__



namespace carpc::service::secure::__private__ {

   enum class eRequestStatus : std::uint8_t { BUSY, READY };

   struct RequestInfo
   {
      RequestInfo( const comm::sequence::ID _server_seq_id )
         : server_seq_id( _server_seq_id )
      { }
      RequestInfo( const comm::sequence::ID _server_seq_id, const comm::sequence::ID _client_seq_id, const Address _client_addr )
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

} // namespace carpc::service::secure::__private__



namespace carpc::service::secure::__private__ {

   template< typename TYPES >
   struct NotificationStatus
   {
      struct LocalContext
      {
         using tSet = std::set< LocalContext >;

         bool operator<( const LocalContext& other ) const
         {
            if( tid != other.tid )
               return tid < other.tid;
            return sid < other.sid;
         }

         application::thread::ID    tid;
         comm::service::ID          sid;
      };

      public:
         bool is_subscribed( ) const;
         void add_subscriber( const application::Context&, const comm::service::ID& );
         void remove_subscriber( const application::Context&, const comm::service::ID& );
         template< typename tNotificationData, typename... Args >
            void notify_subscribers( const TServer< TYPES >&, const Args& ... args );

      public:
         std::shared_ptr< typename TYPES::tBaseData > data( ) const;
      private:
         std::map< application::process::ID, typename LocalContext::tSet > subscribers;
         std::shared_ptr< typename TYPES::tBaseData > mp_data = nullptr;
   };

   template< typename TYPES >
   std::shared_ptr< typename TYPES::tBaseData > NotificationStatus< TYPES >::data( ) const
   {
      return mp_data;
   }

   template< typename TYPES >
   bool NotificationStatus< TYPES >::is_subscribed( ) const
   {
      return !( subscribers.empty( ) );
   }

   template< typename TYPES >
   void NotificationStatus< TYPES >::add_subscriber( const application::Context& context, const comm::service::ID& service_id )
   {
      // In case of current process PID can have two different exact values but what can mean the same logic values.
      // It is current process ID and appliocation::process::local
      // Here we should standartize what exact value should be stored in collection.
      application::process::ID pid = application::process::local;
      if( context.is_external( ) )
         pid = context.pid( );

      auto iterator = subscribers.find( pid );
      if( subscribers.end( ) == iterator )
         subscribers.emplace( pid, typename LocalContext::tSet{ LocalContext{ context.tid( ), service_id } } );
      else
         iterator->second.emplace( LocalContext{ context.tid( ), service_id } );
   }

   template< typename TYPES >
   void NotificationStatus< TYPES >::remove_subscriber( const application::Context& context, const comm::service::ID& service_id )
   {
      // In case of current process PID can have two different exact values but what can mean the same logic values.
      // It is current process ID and appliocation::process::local
      // Here we should standartize what exact value should be stored in collection.
      application::process::ID pid = application::process::local;
      if( context.is_external( ) )
      {
         pid = context.pid( );
      }

      auto iterator = subscribers.find( pid );
      if( subscribers.end( ) == iterator || iterator->second.empty( ) )
      {
         SYS_WRN( "removing not existing subscriber" );
      }
      else
         iterator->second.erase( LocalContext{ context.tid( ), service_id } );
   }

   template< typename TYPES >
   template< typename tNotificationData, typename... Args >
   void NotificationStatus< TYPES >::notify_subscribers( const TServer< TYPES >& server, const Args& ... args )
   {
      mp_data = std::make_shared< tNotificationData >( args... );

      if( false == is_subscribed( ) )
      {
         SYS_VRB( "there are no subscribers to notify: %s", tNotificationData::NOTIFICATION.c_str( ) );
         return;
      }

      typename TYPES::tEventData event_data( mp_data );

      // Notifying all subscribers by sending notification events to each consumer in each process.
      for( const auto& pair : subscribers )
      {
         for( const auto& subscriber : pair.second )
         {
            typename TYPES::tEventUserSignature event_signature(
               server.signature( ).role( ), tNotificationData::NOTIFICATION, server.id( ), subscriber.sid
            );
            TYPES::tEvent::create( event_signature )->
               data( event_data )->send( application::Context( subscriber.tid, pair.first ) );
         }
      }
   }

} // namespace carpc::service::secure::__private__



namespace carpc::service::secure::__private__ {

   template< typename TYPES >
   class TServer
      : public IServer
      , public TYPES::tEventConsumer
   {
      using tServer = TServer< TYPES >;
      using tRequestResponseIDs = RequestResponseIDs< typename TYPES::tEventID >;
      using tRequestStatusMap = std::map< tRequestResponseIDs, RequestStatus, typename tRequestResponseIDs::Comparator::ByRequest >;
      using tNotificationStatus = NotificationStatus< TYPES >;
      using tAttributeStatusMap = std::map< typename TYPES::tEventID, tNotificationStatus >;

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
         template< typename tNotificationData, typename... Args >
            void notify( const Args&... args );
         template< typename tNotificationData >
            const tNotificationData* attribute( ) const;
         template< typename tRequestData >
            const tRequestData* get_event_data( const typename TYPES::tEvent& event );

      private:
         void process_event( const typename TYPES::tEvent& ) override final;
      private:
         virtual void process_request_event( const typename TYPES::tEvent& ) = 0;
         bool prepare_request( const typename TYPES::tEvent& );
         template< typename tResponseData >
            std::optional< RequestInfo > prepare_response( );
         bool process_subscription( const typename TYPES::tEvent& );
      public:
         const comm::sequence::ID unblock_request( );
         void prepare_response( const comm::sequence::ID );

      private:
         tRequestStatusMap                         m_request_status_map;
         comm::sequence::ID                        m_seq_id = comm::sequence::ID::zero;
         std::optional< typename TYPES::tEventID > m_processing_event_id = std::nullopt;
         std::optional< comm::sequence::ID >       m_processing_seq_id = std::nullopt;
         tAttributeStatusMap                       m_attribute_status_map;
   };




   template< typename TYPES >
   TServer< TYPES >::TServer( const carpc::async::tAsyncTypeID& interface_type_id, const std::string& role_name, const bool is_export )
      : IServer( interface_type_id, role_name, is_export )
      , TYPES::tEventConsumer( )
   {
      for( auto rr_item : TYPES::RR )
      {
         m_request_status_map.emplace( rr_item, RequestStatus{ } );
      }
      for( auto n_item : TYPES::N )
      {
         m_attribute_status_map.emplace( n_item.notification, tNotificationStatus{ } );
      }
   }

   template< typename TYPES >
   TServer< TYPES >::~TServer( )
   {
      TYPES::tEvent::clear_all_notifications( this );
   }

   template< typename TYPES >
   void TServer< TYPES >::connected( const Address& proxy_address )
   {
      for( auto rr_item : TYPES::RR )
      {
         // @TDA-BUG: Here and in similar places could be a problem because we subscribe for notification from client just right now.
         // This means that we process event about connection status for new client.
         // But in case if service thread where this client is created has more time quantums for running this will lead to situation
         // when notification event about connected server will be processed by this client => this client can send some request
         // to the server from method "connected" => corresponding event will be created and sent but will not be inserted to current
         // service thread where this server is created because  "set_notification" method below is not called yet.
         // This issue is not relevant to "fast" implementation.
         TYPES::tEvent::set_notification(
            this,
            typename TYPES::tEventUserSignature( signature( ).role( ), rr_item.request, proxy_address.id( ), id( ) )
         );
      }
      for( auto n_item : TYPES::N )
      {
         TYPES::tEvent::set_notification(
            this,
            typename TYPES::tEventUserSignature( signature( ).role( ), n_item.subscribe, proxy_address.id( ), id( ) )
         );
         TYPES::tEvent::set_notification(
            this,
            typename TYPES::tEventUserSignature( signature( ).role( ), n_item.unsubscribe, proxy_address.id( ), id( ) )
         );
      }

      connected( );
   }

   template< typename TYPES >
   void TServer< TYPES >::disconnected( const Address& proxy_address )
   {
      for( auto rr_item : TYPES::RR )
      {
         TYPES::tEvent::clear_notification(
            this,
            typename TYPES::tEventUserSignature( signature( ).role( ), rr_item.request, proxy_address.id( ), id( ) )
         );
      }
      for( auto n_item : TYPES::N )
      {
         TYPES::tEvent::clear_notification(
            this,
            typename TYPES::tEventUserSignature( signature( ).role( ), n_item.subscribe, proxy_address.id( ), id( ) )
         );
         TYPES::tEvent::clear_notification(
            this,
            typename TYPES::tEventUserSignature( signature( ).role( ), n_item.unsubscribe, proxy_address.id( ), id( ) )
         );
      }

      disconnected( );
   }

   template< typename TYPES >
   void TServer< TYPES >::process_event( const typename TYPES::tEvent& event )
   {
      SYS_VRB( "processing event: %s", event.info( ).dbg_name( ).c_str( ) );
      m_processing_event_id = event.info( ).id( );

      if( true == prepare_request( event ) )
      {
         process_request_event( event );
      }
      else if( true == process_subscription( event ) )
      {
      }
      else
      {
         SYS_WRN( "not processable event: %s", event.info( ).dbg_name( ).c_str( ) );
      }

      m_processing_event_id.reset( );
   }

   template< typename TYPES >
   bool TServer< TYPES >::prepare_request( const typename TYPES::tEvent& event )
   {
      const typename TYPES::tEventID event_id = event.info( ).id( );
      const comm::service::ID from_id = event.info( ).from( );
      const comm::service::ID to_id = event.info( ).to( );
      const comm::sequence::ID seq_id = event.info( ).seq_id( );
      const auto from_context = event.context( );

      // Find request event_id in request status map
      auto iterator_status_map = m_request_status_map.find( tRequestResponseIDs( event_id ) );
      if( m_request_status_map.end( ) == iterator_status_map )
      {
         SYS_WRN( "not a request ID: %s", event_id.c_str( ) );
         return false;
      }
      auto& rrIDs = iterator_status_map->first;
      auto& request_status = iterator_status_map->second;

      // Check if received request has connected response.
      // If current request does not have connected response request status should not be set to busy and all data for response is not needed.
      if( TYPES::tEventID::Undefined == rrIDs.response )
         return true;

      // Check request status for current request ID
      if( eRequestStatus::BUSY == request_status.status )
      {
         SYS_WRN( "request busy: %s", event_id.c_str( ) );
         // Sending event with request busy id
         TYPES::tEvent::create(
            typename TYPES::tEventUserSignature(
                  signature( ).role( ), rrIDs.busy, id( ), from_id, seq_id
               )
         )->send( from_context );
         return false;
      }

      // Set request busy for current request ID
      request_status.status = eRequestStatus::BUSY;
      // Increment common sequence ID and set it's value to current processing sequence ID for current request ID
      request_status.processing_server_seq_id = ++m_seq_id;
      // Store RequestInfo structure to request info set 
      request_status.info_set.emplace( m_seq_id, seq_id, Address( from_context, from_id ) );
      return true;
   }

   template< typename TYPES >
   template< typename tResponseData >
   std::optional< RequestInfo > TServer< TYPES >::prepare_response( )
   {
      // Find request id in request status map
      auto iterator_status_map = m_request_status_map.find( tRequestResponseIDs( tResponseData::REQUEST ) );
      if( m_request_status_map.end( ) == iterator_status_map )
      {
         SYS_WRN( "not a request ID: %s", tResponseData::REQUEST.c_str( ) );
         return std::nullopt;
      }
      auto& request_status = iterator_status_map->second;
      auto& request_info_set = request_status.info_set;

      comm::sequence::ID serching_seq_id = m_processing_seq_id.value_or( request_status.processing_server_seq_id );
      // Search for RequestInfo structure in request infor set for current request ID
      auto iterator_info_set = request_info_set.find( RequestInfo{ serching_seq_id } );
      if( request_info_set.end( ) == iterator_info_set )
      {
         SYS_WRN( "request info for currnet sequence id has not been found: %s", serching_seq_id.dbg_name( ).c_str( ) );
         return std::nullopt;
      }

      RequestInfo request_info = *iterator_info_set;

      request_info_set.erase( iterator_info_set );
      request_status.status = eRequestStatus::READY;
      m_processing_seq_id.reset( );

      return request_info;
   }

   template< typename TYPES >
   const comm::sequence::ID TServer< TYPES >::unblock_request( )
   {
      if( std::nullopt == m_processing_event_id )
      {
         SYS_WRN( "unable unblock request" );
         return comm::sequence::ID::invalid;
      }

      // Find request id in request status map
      auto iterator_status_map = m_request_status_map.find( tRequestResponseIDs( m_processing_event_id.value( ) ) );
      if( m_request_status_map.end( ) == iterator_status_map )
      {
         SYS_WRN( "not a request ID: %s", m_processing_event_id.value( ).c_str( ) );
         return comm::sequence::ID::invalid;
      }
      auto& request_status = iterator_status_map->second;

      request_status.status = eRequestStatus::READY;
      return request_status.processing_server_seq_id;
   }

   template< typename TYPES >
   void TServer< TYPES >::prepare_response( const comm::sequence::ID seq_id )
   {
      m_processing_seq_id = seq_id;
   }

   template< typename TYPES >
   template< typename tResponseData, typename... Args >
   void TServer< TYPES >::response( const Args&... args )
   {
      const std::optional< RequestInfo > request_info_opt = prepare_response< tResponseData >( );
      if( std::nullopt == request_info_opt )
         return;

      const RequestInfo& request_info = request_info_opt.value( );

      typename TYPES::tEventUserSignature event_signature(
         signature( ).role( ), tResponseData::RESPONSE, id( ), request_info.client_addr.id( ), request_info.client_seq_id
      );
      typename TYPES::tEventData event_data( std::make_shared< tResponseData >( args... ) );
      TYPES::tEvent::create( event_signature )->
         data( event_data )->send( request_info.client_addr.context( ) );
   }

   template< typename TYPES >
   bool TServer< TYPES >::process_subscription( const typename TYPES::tEvent& event )
   {
      const typename TYPES::tEventID event_id = event.info( ).id( );
      const comm::service::ID from_id = event.info( ).from( );
      const auto from_context = event.context( );

      for( auto& item : TYPES::N )
      {
         if( item.subscribe != event_id && item.unsubscribe != event_id )
            continue;

         auto iterator = m_attribute_status_map.find( item.notification );
         if( m_attribute_status_map.end( ) == iterator )
         {
            SYS_WRN( "was not found notification ID: %s", item.notification.c_str( ) );
            return false;
         }

         SYS_VRB( "notification subscribe/unsubscribe ID: %s", event_id.c_str( ) );
         tNotificationStatus& notification_status = iterator->second;
         if( item.subscribe == event_id )
         {
            notification_status.add_subscriber( from_context, from_id );

            if( notification_status.data( ) )
            {
               typename TYPES::tEventUserSignature event_signature( signature( ).role( ), item.notification, id( ), from_id );
               typename TYPES::tEventData event_data( notification_status.data( ) );
               TYPES::tEvent::create( event_signature )->
                  data( event_data )->send( from_context );
            }
         }
         else if( item.unsubscribe == event_id )
         {
            notification_status.remove_subscriber( from_context, from_id );
         }

         return true;
      }

      SYS_WRN( "not a notification subscribe/unsubscribe ID: %s", event_id.c_str( ) );
      return false;
   }

   template< typename TYPES >
   template< typename tNotificationData, typename... Args >
   void TServer< TYPES >::notify( const Args&... args )
   {
      auto iterator = m_attribute_status_map.find( tNotificationData::NOTIFICATION );
      if( m_attribute_status_map.end( ) == iterator )
      {
         SYS_WRN( "not a notification ID: %s", tNotificationData::NOTIFICATION.c_str( ) );
         return;
      }

      tNotificationStatus& notification_status = iterator->second;
      notification_status.template notify_subscribers< tNotificationData>( *this, args... );
   }

   template< typename TYPES >
   template< typename tNotificationData >
   const tNotificationData* TServer< TYPES >::attribute( ) const
   {
      auto iterator = m_attribute_status_map.find( tNotificationData::NOTIFICATION );
      if( m_attribute_status_map.end( ) == iterator )
      {
         SYS_WRN( "not a notification ID: %s", tNotificationData::NOTIFICATION.c_str( ) );
         return nullptr;
      }

      return static_cast< tNotificationData* >( iterator->second.data( ).get( ) );
   }

   template< typename TYPES >
   template< typename tRequestData >
   const tRequestData* TServer< TYPES >::get_event_data( const typename TYPES::tEvent& event )
   {
      if( const tRequestData* p_data = static_cast< tRequestData* >( event.data( )->ptr.get( ) ) )
         return p_data;

      SYS_ERR( "missing request data for request ID: %s", event.info( ).id( ).c_str( ) );
      return nullptr;
   }

} // namespace carpc::service::secure::__private__



namespace carpc::service::secure {

   template< typename TYPES >
   class TServer : public __private__::TServer< TGenerator< TYPES > >
   {
      public:
         TServer( const std::string&, const bool );

         using tService = typename TGenerator< TYPES >::Service;
   };

   template< typename TYPES >
   TServer< TYPES >::TServer( const std::string& role_name, const bool is_export )
      : __private__::TServer< TGenerator< TYPES > >( TGenerator< TYPES >::interface_type_id, role_name, is_export )
   {
      REGISTER_EVENT( tService );
   }

} // namespace carpc::service::secure



#undef CLASS_ABBR
