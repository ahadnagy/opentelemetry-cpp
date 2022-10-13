//
// Created by Hadnagy Ákos on 2022. 10. 12..
//
// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef ENABLE_METRICS_PREVIEW

#  include <chrono>
#  include <memory>
#  include <thread>
#  include <vector>
#  include "opentelemetry/metrics/provider.h"
#  include "opentelemetry/nostd/shared_ptr.h"

#  include "metrics_foo_library/foo_library.h"
#  include "opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h"
#  include "opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader.h"
#  include "opentelemetry/sdk/metrics/meter.h"
#  include "opentelemetry/sdk/metrics/meter_provider.h"

namespace metric_sdk    = opentelemetry::sdk::metrics;
namespace nostd         = opentelemetry::nostd;
namespace common        = opentelemetry::common;
namespace metrics_api   = opentelemetry::metrics;
namespace otlp_exporter = opentelemetry::exporter::otlp;

namespace
{

otlp_exporter::OtlpGrpcMetricExporterOptions options;

using SyncInstrumentHandle =
    opentelemetry::nostd::shared_ptr<opentelemetry::metrics::SynchronousInstrument>;

void initMetrics()
{
  auto exporter = otlp_exporter::OtlpGrpcMetricExporterFactory::Create(options);

  std::string version{"1.2.0"};
  std::string schema{"https://opentelemetry.io/schemas/1.2.0"};

  // Initialize and set the global MeterProvider
  metric_sdk::PeriodicExportingMetricReaderOptions options;
  options.export_interval_millis = std::chrono::milliseconds(50);
  options.export_timeout_millis  = std::chrono::milliseconds(49);
  std::unique_ptr<metric_sdk::MetricReader> reader{
      new metric_sdk::PeriodicExportingMetricReader(std::move(exporter), options)};
  auto provider = std::shared_ptr<metrics_api::MeterProvider>(new metric_sdk::MeterProvider());
  auto p        = std::static_pointer_cast<metric_sdk::MeterProvider>(provider);
  p->AddMetricReader(std::move(reader));

  metrics_api::Provider::SetMeterProvider(provider);
}

struct CallbackHandle {
  opentelemetry::nostd::shared_ptr<opentelemetry::metrics::ObservableInstrument>
      instrument;
  metrics_api::ObservableCallbackPtr callback_ptr;
  void* state;
};



class MeasurementFetcher
{
public:
  static void Fetcher(opentelemetry::metrics::ObserverResult observer_result, void * /* state */)
  {
    if (nostd::holds_alternative<
            nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(observer_result))
    {
      nostd::get<nostd::shared_ptr<opentelemetry::metrics::ObserverResultT<long>>>(
          observer_result)
          ->Observe(0);
    }
  }
};

CallbackHandle AddLongObservableGaugeWithView(
    const std::string &view_name,
    const std::string &instrument_name,
    const std::string &description,
    opentelemetry::metrics::ObservableCallbackPtr cb,
    void *state)
{

  auto provider = metrics_api::Provider::GetMeterProvider();
  nostd::shared_ptr<metrics_api::Meter> meter = provider->GetMeter("test", "");

  std::unique_ptr<metric_sdk::InstrumentSelector> observable_instrument_selector{
      new metric_sdk::InstrumentSelector(
          metric_sdk::InstrumentType::kObservableGauge,
          instrument_name)};

  std::unique_ptr<metric_sdk::MeterSelector> observable_meter_selector{
      new metric_sdk::MeterSelector(view_name, "", "")};

  std::unique_ptr<metric_sdk::View> observable_instrument_view{
      new metric_sdk::View(view_name, description,
                           metric_sdk::AggregationType::kLastValue)};


  auto p = dynamic_cast<metric_sdk::MeterProvider *>(provider.get());
  if (p)
  {
    p->AddView(std::move(observable_instrument_selector),
               std::move(observable_meter_selector),
               std::move(observable_instrument_view));
  }

  auto instrument= meter->CreateLongObservableGauge(instrument_name);
  instrument->AddCallback(MeasurementFetcher::Fetcher, state);
  return CallbackHandle{std::move(instrument), cb, state};
}
}  // namespace

int main(int argc, char *argv[])
{
  // Removing this line will leave the default noop MetricProvider in place.
  initMetrics();
  std::unordered_map<std::string, CallbackHandle> async_instruments;
  std::unordered_map<std::string, SyncInstrumentHandle> sync_instruments;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  for (int i = 0; i < 10; i++) {
    std::string instr_name = "test_"+std::to_string(i);
    auto instr = AddLongObservableGaugeWithView(instr_name,
                                                             instr_name,
                                                             "description",
                                                             MeasurementFetcher::Fetcher, nullptr);
    async_instruments[instr_name] = std::move(instr);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  for (auto it = async_instruments.begin(); it != async_instruments.end();) {
    auto handle = it->second;
    handle.instrument->RemoveCallback(handle.callback_ptr, handle.state);
    it = async_instruments.erase(it);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto provider = metrics_api::Provider::GetMeterProvider();
  auto p = dynamic_cast<metric_sdk::MeterProvider*>(provider.get());
  if(p) {
    p->ForceFlush();
    p->Shutdown();
  }
}
#endif
