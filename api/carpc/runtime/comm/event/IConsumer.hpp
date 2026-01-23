#pragma once



namespace carpc::runtime::comm::event {

   struct IConsumer
   {
      IConsumer( ) = default;
      virtual ~IConsumer( ) = default;
   };

} // namespace carpc::runtime::comm::event
