#pragma once

#include "carpc/runtime/comm/service/IClient.hpp"
#include "carpc/runtime/comm/service/experimental/TProxy.hpp"
#include "carpc/runtime/comm/service/experimental/TGenerator.hpp"

#include "carpc/trace/Trace.hpp"
#define CLASS_ABBR "TClient"



namespace carpc::service::experimental::__private__ {

   template< typename _TGenerator >
   class TClient : public IClient
   {
      using tProxy = TProxy< _TGenerator >;
      using tClient = TClient< _TGenerator >;

      public:
         TClient( const carpc::async::tAsyncTypeID&, const std::string&, const bool );
         ~TClient( ) override;

      public:
         virtual void process_event( const typename _TGenerator::method::tEvent& ) = 0;
         virtual void process_event( const typename _TGenerator::attribute::tEvent& ) = 0;

      protected:
         template< typename tRequestData, typename... Args >
            const comm::sequence::ID request( tClient*, const Args&... );
         template< typename tNotificationData >
            const bool subscribe( tClient* p_client );
         template< typename tNotificationData >
            const bool unsubscribe( tClient* p_client );

      protected:
         template< typename tMethodData >
            const tMethodData* get_event_data( const typename _TGenerator::method::tEvent& event );
         template< typename tAttributeData >
            const tAttributeData* get_event_data( const typename _TGenerator::attribute::tEvent& event );

      private:
         tProxy* proxy( ) const;
         tProxy* mp_proxy = nullptr;
   };



   template< typename _TGenerator >
   TClient< _TGenerator >::TClient(
         const carpc::async::tAsyncTypeID& interface_type_id,
         const std::string& role_name,
         const bool is_import
      )
   {
      mp_proxy = tProxy::create( interface_type_id, role_name, is_import );
      mp_proxy->register_client( this );
      if( mp_proxy )
      {
         SYS_VRB( "created: %s", mp_proxy->signature( ).dbg_name( ).c_str( ) );
      }
      else
      {
         SYS_ERR( "proxy can't be created => client will not work: %s / %s / %s",
                     interface_type_id.dbg_name( ).c_str( ),
                     role_name.c_str( ),
                     is_import ? "import" : " no import"
            );
      }
   }

   template< typename _TGenerator >
   TClient< _TGenerator >::~TClient( )
   {
      if( mp_proxy )
      {
         SYS_VRB( "destroyed: %s", mp_proxy->signature( ).dbg_name( ).c_str( ) );
         mp_proxy->unregister_client( this );
         mp_proxy = nullptr;
      }
   }

   template< typename _TGenerator >
   template< typename tRequestData, typename... Args >
   const comm::sequence::ID TClient< _TGenerator >::request( tClient* p_client, const Args&... args )
   {
      return mp_proxy->template request< tRequestData >( this, args... );
   }

   template< typename _TGenerator >
   template< typename tAttributeData >
   const bool TClient< _TGenerator >::subscribe( tClient* p_client )
   {
      return mp_proxy->template subscribe< tAttributeData >( this );
   }

   template< typename _TGenerator >
   template< typename tAttributeData >
   const bool TClient< _TGenerator >::unsubscribe( tClient* p_client )
   {
      return mp_proxy->template unsubscribe< tAttributeData >( this );
   }

   template< typename _TGenerator >
   template< typename tMethodData >
   const tMethodData* TClient< _TGenerator >::get_event_data(
         const typename _TGenerator::method::tEvent& event
      )
   {
      if( const tMethodData* p_data = static_cast< tMethodData* >( event.data( )->ptr.get( ) ) )
         return p_data;

      SYS_ERR( "missing data for method ID: %s", event.info( ).id( ).c_str( ) );
      return nullptr;
   }

   template< typename _TGenerator >
   template< typename tAttributeData >
   const tAttributeData* TClient< _TGenerator >::get_event_data(
         const typename _TGenerator::attribute::tEvent& event
      )
   {
      if( const tAttributeData* p_data = static_cast< tAttributeData* >( event.data( )->ptr.get( ) ) )
         return p_data;

      SYS_ERR( "missing data for attribute ID: %s", event.info( ).id( ).c_str( ) );
      return nullptr;
   }

   template< typename _TGenerator >
   typename TClient< _TGenerator >::tProxy* TClient< _TGenerator >::proxy( ) const
   {
      return mp_proxy;
   }

} // namespace carpc::service::experimental::__private__



namespace carpc::service::experimental {

   template< typename _ServiceTypes >
   class TClient : public __private__::TClient< TGenerator< _ServiceTypes > >
   {
      public:
         TClient( const std::string&, const bool );

         using tMethod = typename TGenerator< _ServiceTypes >::Method;
         using tAttribute = typename TGenerator< _ServiceTypes >::Attribute;
   };

   template< typename _ServiceTypes >
   TClient< _ServiceTypes >::TClient( const std::string& role_name, const bool is_export )
      : __private__::TClient< TGenerator< _ServiceTypes > >(
            TGenerator< _ServiceTypes >::interface_type_id, role_name, is_export
         )
   {
      REGISTER_EVENT( tMethod );
      REGISTER_EVENT( tAttribute );
   }

} // namespace carpc::service::experimental



#undef CLASS_ABBR
