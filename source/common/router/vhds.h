#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "envoy/api/v2/rds.pb.h"
#include "envoy/api/v2/route/route.pb.h"
#include "envoy/config/filter/network/http_connection_manager/v2/http_connection_manager.pb.h"
#include "envoy/config/subscription.h"
#include "envoy/http/codes.h"
#include "envoy/local_info/local_info.h"
#include "envoy/router/rds.h"
#include "envoy/router/route_config_update_receiver.h"
#include "envoy/server/filter_config.h"
#include "envoy/singleton/instance.h"
#include "envoy/stats/scope.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/logger.h"
#include "common/config/subscription_factory.h"
#include "common/init/target_impl.h"
#include "common/protobuf/utility.h"

namespace Envoy {
namespace Router {

// clang-format off
#define ALL_VHDS_STATS(COUNTER)                                                                     \
  COUNTER(config_reload)                                                                           \
  COUNTER(update_empty)

// clang-format on

struct VhdsStats {
  ALL_VHDS_STATS(GENERATE_COUNTER_STRUCT)
};

typedef std::unique_ptr<Envoy::Config::Subscription> (*SubscriptionFactoryFunction)(
    const envoy::api::v2::core::ConfigSource&, const LocalInfo::LocalInfo&, Event::Dispatcher&,
    Upstream::ClusterManager&, Envoy::Runtime::RandomGenerator&, Stats::Scope&, const std::string&,
    const std::string&, absl::string_view, ProtobufMessage::ValidationVisitor& validation_visitor,
    Api::Api&, Envoy::Config::SubscriptionCallbacks&);

class VhdsSubscription : Envoy::Config::SubscriptionCallbacks,
                         Logger::Loggable<Logger::Id::router> {
public:
  VhdsSubscription(RouteConfigUpdatePtr& config_update_info,
                   Server::Configuration::FactoryContext& factory_context,
                   const std::string& stat_prefix,
                   std::unordered_set<RouteConfigProvider*>& route_config_providers,
                   SubscriptionFactoryFunction factory_function =
                       Envoy::Config::SubscriptionFactory::subscriptionFromConfigSource);
  ~VhdsSubscription() override { init_target_.ready(); }

  // Config::SubscriptionCallbacks
  // TODO(fredlas) deduplicate
  void onConfigUpdate(const Protobuf::RepeatedPtrField<ProtobufWkt::Any>&,
                      const std::string&) override {
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
  void onConfigUpdate(const Protobuf::RepeatedPtrField<envoy::api::v2::Resource>&,
                      const Protobuf::RepeatedPtrField<std::string>&, const std::string&) override;
  void onConfigUpdateFailed(const EnvoyException* e) override;
  std::string resourceName(const ProtobufWkt::Any& resource) override {
    return MessageUtil::anyConvert<envoy::api::v2::route::VirtualHost>(resource,
                                                                       validation_visitor_)
        .name();
  }
  void registerInitTargetWithInitManager(Init::Manager& m) { m.add(init_target_); }

  RouteConfigUpdatePtr& config_update_info_;
  std::unique_ptr<Envoy::Config::Subscription> subscription_;
  Init::TargetImpl init_target_;
  Stats::ScopePtr scope_;
  VhdsStats stats_;
  std::unordered_set<RouteConfigProvider*>& route_config_providers_;
  ProtobufMessage::ValidationVisitor& validation_visitor_;
};

using VhdsSubscriptionPtr = std::unique_ptr<VhdsSubscription>;

} // namespace Router
} // namespace Envoy
