#pragma once

#include "carpc/runtime/comm/event/TGenerator.hpp"




#define DEFINE_EVENT_BASE(                                           \
         scopeType, serviceType, eventType, dataType, signatureType  \
      )                                                              \
                                                                     \
   scopeType eventType {                                             \
      class eventType##_TYPE;                                        \
      using Generator   = carpc::runtime::comm::event::TGenerator<   \
         serviceType, eventType##_TYPE, dataType, signatureType >;   \
      using Event       = typename Generator::Config::tEvent;        \
      using Signature   = typename Generator::Config::tSignature;    \
      using Data        = typename Generator::Config::tData;         \
      using Consumer    = typename Generator::Config::tConsumer;     \
   }



#define DEFINE_EVENT_N( eventType, dataType, signatureType )      \
   DEFINE_EVENT_BASE(                                             \
         namespace,                                               \
         carpc::runtime::comm::event::ServiceType::NO_IPC,        \
         eventType,                                               \
         dataType,                                                \
         signatureType                                            \
      )

#define DEFINE_IPC_EVENT_N( eventType, dataType, signatureType )  \
   DEFINE_EVENT_BASE(                                             \
         namespace,                                               \
         carpc::runtime::comm::event::ServiceType::IPC,           \
         eventType,                                               \
         dataType,                                                \
         signatureType                                            \
      )



#define DEFINE_EVENT_S( eventType, dataType, signatureType )      \
   DEFINE_EVENT_BASE(                                             \
         struct,                                                  \
         carpc::runtime::comm::event::ServiceType::NO_IPC,        \
         eventType,                                               \
         dataType,                                                \
         signatureType                                            \
      )

#define DEFINE_IPC_EVENT_S( eventType, dataType, signatureType )  \
   DEFINE_EVENT_BASE(                                             \
         struct,                                                  \
         carpc::runtime::comm::event::ServiceType::IPC,           \
         eventType,                                               \
         dataType,                                                \
         signatureType                                            \
      )



#define DEFINE_EVENT                   DEFINE_EVENT_S

#define DEFINE_IPC_EVENT               DEFINE_IPC_EVENT_S
