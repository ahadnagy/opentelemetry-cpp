// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifndef ENABLE_METRICS_PREVIEW
#  include <vector>
#  include "opentelemetry/sdk/metrics/data/exemplar_data.h"
#  include "opentelemetry/sdk/metrics/exemplar/filter.h"
#  include "opentelemetry/sdk/metrics/exemplar/reservoir_cell_selector.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace sdk
{
namespace metrics
{
/**
 * An interface for an exemplar reservoir of samples.
 *
 * <p>This represents a reservoir for a specific "point" of metric data.
 */
class ExemplarReservoir
{
public:
  virtual ~ExemplarReservoir() = default;

  /** Offers a long measurement to be sampled. */
  virtual void OfferMeasurement(
      int64_t value,
      const MetricAttributes &attributes,
      const opentelemetry::context::Context &context,
      const opentelemetry::common::SystemTimestamp &timestamp) noexcept = 0;

  /** Offers a double measurement to be sampled. */
  virtual void OfferMeasurement(
      double value,
      const MetricAttributes &attributes,
      const opentelemetry::context::Context &context,
      const opentelemetry::common::SystemTimestamp &timestamp) noexcept = 0;

  /**
   * Builds vector of Exemplars for exporting from the current reservoir.
   *
   * <p>Additionally, clears the reservoir for the next sampling period.
   *
   * @param pointAttributes the Attributes associated with the metric point.
   *     ExemplarDatas should filter these out of their final data state.
   * @return A vector of sampled exemplars for this point. Implementers are expected to
   *     filter out pointAttributes from the original recorded attributes.
   */
  virtual std::vector<std::shared_ptr<ExemplarData>> CollectAndReset(
      const MetricAttributes &pointAttributes) noexcept = 0;

  static nostd::shared_ptr<ExemplarReservoir> GetFilteredExemplarReservoir(
      std::shared_ptr<ExemplarFilter> filter,
      std::shared_ptr<ExemplarReservoir> reservoir);

  static nostd::shared_ptr<ExemplarReservoir> GetHistogramExemplarReservoir(
      size_t size,
      std::shared_ptr<ReservoirCellSelector> reservoir_cell_selector,
      std::shared_ptr<ExemplarData> (ReservoirCell::*map_and_reset_cell)(
          const common::OrderedAttributeMap &attributes));

  static nostd::shared_ptr<ExemplarReservoir> GetNoExemplarReservoir();
};

}  // namespace metrics
}  // namespace sdk
OPENTELEMETRY_END_NAMESPACE
#endif
