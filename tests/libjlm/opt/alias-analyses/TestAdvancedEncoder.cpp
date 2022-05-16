/*
 * Copyright 2020 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "AliasAnalysesTests.hpp"

#include <test-registry.hpp>

#include <jlm/opt/alias-analyses/AdvancedEncoder.hpp>
#include <jlm/opt/alias-analyses/PointsToGraph.hpp>
#include <jlm/opt/alias-analyses/Steensgaard.hpp>
#include <jlm/util/Statistics.hpp>

static std::unique_ptr<jlm::aa::PointsToGraph>
RunSteensgaard(jlm::RvsdgModule & rvsdgModule)
{
  using namespace jlm;

  aa::Steensgaard steensgaard;
  StatisticsDescriptor statisticsDescriptor;
  return steensgaard.Analyze(rvsdgModule, statisticsDescriptor);
}

static void
RunAdvancedEncoder(
  jlm::aa::PointsToGraph & pointsToGraph,
  jlm::RvsdgModule & rvsdgModule)
{
  using namespace jlm;

  StatisticsDescriptor statisticsDescriptor;
  aa::AdvancedEncoder::Encode(pointsToGraph, rvsdgModule, statisticsDescriptor);
}

static void
TestStore1()
{
  auto ValidateRvsdg = [](const StoreTest1 & test)
  {
  };

  StoreTest1 test;
  // jive::view(test.graph().root(), stdout);

  auto pointsToGraph = RunSteensgaard(test.module());
  // std::cout << jlm::aa::PointsToGraph::ToDot(*PointsToGraph);

  RunAdvancedEncoder(*pointsToGraph, test.module());
  // jive::view(test.graph().root(), stdout);
  ValidateRvsdg(test);
}

static int
TestAdvancedEncoder()
{
  TestStore1();
  /*
  TestStore2();

  TestLoad1();
  TestLoad2();
  TestLoadFromUndef();

  TestCall1();
  TestCall2();
  TestIndirectCall();

  TestGamma();
  TestTheta();

  TestDelta1();
  TestDelta2();

  TestImports();

  TestPhi();
*/
  return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/opt/alias-analyses/TestAdvancedEncoder", TestAdvancedEncoder)