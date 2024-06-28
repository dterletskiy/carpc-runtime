#include "carpc/runtime/comm/async/Types.hpp"


namespace carpc::async {

   const char* c_str( const eAsyncType async_type )
   {
      switch( async_type )
      {
         case eAsyncType::EVENT:       return "carpc::eAsyncType::EVENT";
         case eAsyncType::RUNNABLE:    return "carpc::eAsyncType::RUNNABLE";
         case eAsyncType::CALLABLE:    return "carpc::eAsyncType::CALLABLE";
         default:                      return "carpc::eAsyncType::UNEFINED";
      }
      return "carpc::eAsyncType::UNEFINED";
   }

   const char* name( const eAsyncType async_type )
   {
      switch( async_type )
      {
         case eAsyncType::EVENT:       return "event";
         case eAsyncType::RUNNABLE:    return "runnable";
         case eAsyncType::CALLABLE:    return "callable";
         default:                      return "unefined";
      }
      return "unefined";
   }

}
