/*
 * Copyright 2022 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/common.hpp>
#include <jlm/ir/operators/lambda.hpp>
#include <jlm/opt/alias-analyses/AdvancedEncoder.hpp>

#include <jive/rvsdg/node.hpp>
#include <jive/rvsdg/traverser.hpp>

#include <typeindex>
#include <functional>

namespace jlm::aa {

AdvancedEncoder::~AdvancedEncoder() noexcept
= default;

AdvancedEncoder::AdvancedEncoder(PointsToGraph & pointsToGraph)
  : PointsToGraph_(pointsToGraph)
{}

void
AdvancedEncoder::Encode(
  PointsToGraph & pointsToGraph,
  RvsdgModule & rvsdgModule,
  const StatisticsDescriptor & statisticsDescriptor)
{
  /*
  AdvancedEncoder advancedEncoder(pointsToGraph);
  advancedEncoder.Encode(rvsdgModule, statisticsDescriptor);
  */
}

void
AdvancedEncoder::Encode(
  RvsdgModule & rvsdgModule,
  const StatisticsDescriptor & statisticsDescriptor)
{}

void
AdvancedEncoder::ComputeRoutingPlan(const RvsdgModule & rvsdgModule)
{

}

void
AdvancedEncoder::ComputeRoutingPlan(jive::region & region)
{
  using namespace jive;

  topdown_traverser traverser(&region);
  for (auto & node : traverser) {
    if (auto simpleNode = dynamic_cast<const simple_node*>(node))
      continue;

    auto structuralNode = AssertedCast<structural_node>(node);
    ComputeRoutingPlan(*structuralNode);
  }
}

void
AdvancedEncoder::ComputeRoutingPlan(const jive::simple_node & simpleNode)
{
  auto computeRoutingPlanStore = [](auto & advancedEncoder, auto & simpleNode)
  {
    advancedEncoder.ComputeRoutingPlan(*AssertedCast<const StoreNode>(&simpleNode));
  };

  static std::unordered_map<
    std::type_index,
    std::function<void(AdvancedEncoder&, const jive::simple_node&)>
  > nodes({
            {typeid(StoreOperation), EncodeStore},
          });

  auto & op = simpleNode.operation();
  if (nodes.find(typeid(op)) == nodes.end())
    return;

  nodes[typeid(op)](*this, simpleNode);
}

void
AdvancedEncoder::ComputeRoutingPlan(const jive::structural_node & structuralNode)
{
  auto computeRoutingPlanLambda = [](auto & advancedEncoder, auto & structuralNode)
  {
    advancedEncoder.ComputeRoutingPlan(*AssertedCast<lambda::node>(&structuralNode));
  };

  static std::unordered_map<
    std::type_index,
    std::function<void(AdvancedEncoder&, jive::structural_node&)>
  > nodes({
            {typeid(lambda::operation), computeRoutingPlanLambda }
          });

  auto & operation = structuralNode.operation();
  JLM_ASSERT(nodes.find(typeid(operation)) != nodes.end());
  nodes[typeid(operation)](*this, structuralNode);
}

void
AdvancedEncoder::ComputeRoutingPlan(const lambda::node & lambdaNode)
{
  ComputeRoutingPlan(*lambdaNode.subregion());
}

void
AdvancedEncoder::ComputeRoutingPlan(const StoreNode & storeNode)
{

}

}