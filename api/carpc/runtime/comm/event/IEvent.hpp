#pragma once

#include "carpc/base/memory/RefCounted.hpp"
#include "carpc/runtime/comm/event/Types.hpp"



namespace carpc::runtime::comm::event {

   class IConsumer;

   class IEvent
      : public carpc::memory::RefCounted
   {
      public:
         IEvent( ) = default;
         virtual ~IEvent( ) = default;

      public:
         virtual void process( IConsumer* p_consumer ) const = 0;

      public:
         const tContext& context( ) const;
         const tPriority& priority( ) const;
         void priority( const tPriority& value );

      protected:
         void on_zero_ref( ) noexcept override;

      protected:
         const tContext m_context = tContext::current( );
         tPriority      m_priority = priority::DEFAULT;
   };

   inline
   const tContext& IEvent::context( ) const
   {
      return m_context;
   }

   inline
   const tPriority& IEvent::priority( ) const
   {
      return m_priority;
   }

   inline
   void IEvent::priority( const tPriority& value )
   {
      m_priority = value;
   }

   inline
   void IEvent::on_zero_ref( ) noexcept
   {
      delete this;
   }


} // namespace carpc::runtime::comm::event
