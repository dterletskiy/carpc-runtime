#pragma once

#include "carpc/runtime/core/Types.hpp"



namespace carpc::runtime::core {

   class Context
   {
      public:
         static const Context invalid;
         static const Context& internal_local( );
         static const Context& internal_broadcast( );
         static const Context& current( );

      public:
         Context( ) = default;
         Context( const process::ID& pid, const thread::ID& tid );
         Context( const Context& ) = default;
         ~Context( ) = default;

      public:
         Context& operator=( const Context& ) = default;
         bool operator==( const Context& ) const;
         bool operator!=( const Context& ) const;
         bool operator<( const Context& ) const;

      public:
         [[nodiscard]] bool is_internal( ) const;
         [[nodiscard]] bool is_internal_local( ) const;
         [[nodiscard]] bool is_internal_broadcast( ) const;
         [[nodiscard]] bool is_external( ) const;
         [[nodiscard]] bool is_valid( ) const;

      public:
         const process::ID& pid( ) const;
         const thread::ID& tid( ) const;
      private:
         const process::ID m_pid = process::ID::invalid;
         const thread::ID m_tid = thread::ID::invalid;
   };



   inline
   const process::ID& Context::pid( ) const
   {
      return m_pid;
   }

   inline
   const thread::ID& Context::tid( ) const
   {
      return m_tid;
   }

} // namespace carpc::runtime::core
