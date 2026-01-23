#pragma once

#include "carpc/base/types/Priority.hpp"
#include "carpc/base/utils/generate/type_id.hpp"
#include "carpc/runtime/core/Context.hpp"



namespace carpc::runtime::comm::event {

   namespace detail {

      template< typename TYPE >
      class TBaseTypeID
      {
         public:
            TBaseTypeID( ) = default;
            TBaseTypeID( const TYPE& _value )
               : m_value( _value )
            {
            }
            TBaseTypeID( const TBaseTypeID< TYPE >& _other )
               : m_value( _other.m_value )
            {
            }
            ~TBaseTypeID( ) = default;

         public:
            bool operator==( const TBaseTypeID< TYPE >& type_id ) const
            {
               return m_value == type_id.m_value;
            }
            bool operator!=( const TBaseTypeID< TYPE >& type_id ) const
            {
               return m_value != type_id.m_value;
            }
            bool operator<( const TBaseTypeID< TYPE >& type_id ) const
            {
               return m_value < type_id.m_value;
            }

         public:
            const TYPE& value( ) const
            {
               return m_value;
            }
         protected:
            const TYPE m_value;
      };

      template< typename T >
      class TTypeID
         : public TBaseTypeID< T >
      {
      };

      template< >
      class TTypeID< std::string >
         : public TBaseTypeID< std::string >
      {
         public:
            using UNDERLYING_TYPE = std::string;
            using TYPE = TTypeID< UNDERLYING_TYPE >;

         public:
            TTypeID( ) = default;
            TTypeID( const UNDERLYING_TYPE& _value )
               : TBaseTypeID< UNDERLYING_TYPE >( _value )
            {
            }
            TTypeID( const TTypeID< UNDERLYING_TYPE >& _other )
               : TBaseTypeID< UNDERLYING_TYPE >( _other.m_value )
            {
            }
            ~TTypeID( ) = default;

         public:
            template< typename T >
            static const UNDERLYING_TYPE& generate( )
            {
               static const UNDERLYING_TYPE& value =
                  carpc::utils::generate::type_id::name< T >( );
               return  value;
            }
      };

      template< >
      class TTypeID< std::uint64_t >
         : public TBaseTypeID< std::uint64_t >
      {
            using UNDERLYING_TYPE = std::uint64_t;
            using TYPE = TTypeID< UNDERLYING_TYPE >;

         public:
            TTypeID( ) = default;
            TTypeID( const UNDERLYING_TYPE& _value )
               : TBaseTypeID< UNDERLYING_TYPE >( _value )
            {
            }
            TTypeID( const TTypeID< UNDERLYING_TYPE >& _other )
               : TBaseTypeID< UNDERLYING_TYPE >( _other.m_value )
            {
            }
            ~TTypeID( ) = default;

         public:
            template< typename T >
            static UNDERLYING_TYPE generate( )
            {
               static const UNDERLYING_TYPE value =
                  carpc::utils::generate::type_id::hash< T >( );
               return value;
            }
      };

   }

   namespace ServiceType
   {
      class NO_IPC;
      class IPC;
   };

   class Event;
   using tPriority = carpc::TPriority< Event, std::uint64_t >;
   using tTypeID = detail::TTypeID< std::string >;
   using tContext = core::Context;

   namespace priority {

      const tPriority MIN = tPriority::min;
      const tPriority DEFAULT = tPriority( 100 );
      const tPriority TIMER = tPriority( 200 );
      const tPriority MAX = tPriority::max;

   }

} // namespace carpc::runtime::comm
