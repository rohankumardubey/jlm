/*
 * Copyright 2022 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_OPT_ALIAS_ANALYSES_ADVANCEDENCODER_HPP
#define JLM_OPT_ALIAS_ANALYSES_ADVANCEDENCODER_HPP

#include <jlm/opt/alias-analyses/MemoryStateEncoder.hpp>

namespace jlm::aa {

class AdvancedEncoder final : public MemoryStateEncoder {
public:
  ~AdvancedEncoder() noexcept override;

  explicit
  AdvancedEncoder(PointsToGraph & pointsToGraph);

  AdvancedEncoder(const AdvancedEncoder &) = delete;

  AdvancedEncoder(AdvancedEncoder &&) = delete;

  AdvancedEncoder &
  operator=(const AdvancedEncoder &) = delete;

  AdvancedEncoder &
  operator=(AdvancedEncoder &&) = delete;

  void
  Encode(
    RvsdgModule & rvsdgModule,
    const StatisticsDescriptor & statisticsDescriptor) override;

  static void
  Encode(
    PointsToGraph & pointsToGraph,
    RvsdgModule & rvsdgModule,
    const StatisticsDescriptor & statisticsDescriptor);

private:
  void
  ComputeRoutingPlan(const RvsdgModule & rvsdgModule);

  void
  ComputeRoutingPlan(jive::region & region);

  void
  ComputeRoutingPlan(const jive::simple_node & advancedEncoder);

  void
  ComputeRoutingPlan(const jive::structural_node & structuralNode);

  void
  ComputeRoutingPlan(const lambda::node & lambdaNode);

  void
  ComputeRoutingPlan(const StoreNode & storeNode);

  [[nodiscard]] const PointsToGraph &
  GetPointsToGraph() const noexcept
  {
    return PointsToGraph_;
  }

  PointsToGraph & PointsToGraph_;
};

}

#endif //JLM_OPT_ALIAS_ANALYSES_ADVANCEDENCODER_HPP