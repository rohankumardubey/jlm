/*
 * Copyright 2019 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm-opt/cmdline.hpp>

#include <jlm/opt/alias-analyses/Optimization.hpp>
#include <jlm/opt/cne.hpp>
#include <jlm/opt/DeadNodeElimination.hpp>
#include <jlm/opt/inlining.hpp>
#include <jlm/opt/InvariantValueRedirection.hpp>
#include <jlm/opt/pull.hpp>
#include <jlm/opt/push.hpp>
#include <jlm/opt/inversion.hpp>
#include <jlm/opt/unroll.hpp>
#include <jlm/opt/reduction.hpp>
#include <jlm/opt/optimization.hpp>

#include <llvm/Support/CommandLine.h>

namespace jlm {

enum class OptimizationId {
  AASteensgaardBasic,
  cne,
  dne,
  iln,
  InvariantValueRedirection,
  psh,
  red,
  ivt,
  url,
  pll,
};

static jlm::optimization *
GetOptimization(enum OptimizationId id)
{
  static jlm::aa::SteensgaardBasic steensgaardBasic;
  static jlm::cne cne;
  static jlm::DeadNodeElimination dne;
  static jlm::fctinline fctinline;
  static jlm::InvariantValueRedirection invariantValueRedirection;
  static jlm::pullin pullin;
  static jlm::pushout pushout;
  static jlm::tginversion tginversion;
  static jlm::loopunroll loopunroll(4);
  static jlm::nodereduction nodereduction;

  static std::unordered_map<OptimizationId, jlm::optimization*>
    map({
          {OptimizationId::AASteensgaardBasic,        &steensgaardBasic},
          {OptimizationId::cne,                       &cne},
          {OptimizationId::dne,                       &dne},
          {OptimizationId::iln,                       &fctinline},
          {OptimizationId::InvariantValueRedirection, &invariantValueRedirection},
          {OptimizationId::pll,                       &pullin},
          {OptimizationId::psh,                       &pushout},
          {OptimizationId::ivt,                       &tginversion},
          {OptimizationId::url,                       &loopunroll},
          {OptimizationId::red,                       &nodereduction}
        });

  JLM_ASSERT(map.find(id) != map.end());
  return map[id];
}

void
parse_cmdline(int argc, char ** argv, jlm::cmdline_options & options)
{
	using namespace llvm;

	/*
		FIXME: The command line parser setup is currently redone
		for every invocation of parse_cmdline. We should be able
		to do it only once and then reset the parser on every
		invocation of parse_cmdline.
	*/

	cl::TopLevelSubCommand->reset();

	cl::opt<std::string> ifile(
	  cl::Positional
	, cl::desc("<input>"));

	cl::opt<std::string> ofile(
	  "o"
	, cl::desc("Write output to <file>")
	, cl::value_desc("file"));

	std::string desc("Write stats to <file>. Default is " + options.sd.filepath().to_str() + ".");
	cl::opt<std::string> sfile(
	  "s"
	, cl::desc(desc)
	, cl::value_desc("file"));

  cl::list<StatisticsDescriptor::StatisticsId> printStatistics(
    cl::values(
        clEnumValN(StatisticsDescriptor::StatisticsId::Aggregation,
                   "print-aggregation-time",
                  "Write aggregation statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::Annotation,
                   "print-annotation-time",
                   "Write annotation statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::BasicEncoderEncoding,
                   "print-basicencoder-encoding",
                   "Write encoding statistics of basic encoder to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::CommonNodeElimination,
                   "print-cne-stat",
                   "Write common node elimination statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::ControlFlowRecovery,
                   "print-cfr-time",
                   "Write control flow recovery statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::DataNodeToDelta,
                   "printDataNodeToDelta",
                   "Write data node to delta node conversion statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::DeadNodeElimination,
                   "print-dne-stat",
                   "Write dead node elimination statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::FunctionInlining,
                   "print-iln-stat",
                   "Write function inlining statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::InvariantValueRedirection,
                   "printInvariantValueRedirection",
                   "Write invariant value redirection statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::JlmToRvsdgConversion,
                   "print-jlm-rvsdg-conversion",
                   "Write Jlm to RVSDG conversion statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::LoopUnrolling,
                   "print-unroll-stat",
                   "Write loop unrolling statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::PullNodes,
                   "print-pull-stat",
                   "Write node pull statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::PushNodes,
                   "print-push-stat",
                   "Write node push statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::ReduceNodes,
                   "print-reduction-stat",
                   "Write node reduction statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::RvsdgConstruction,
                   "print-rvsdg-construction",
                   "Write RVSDG construction statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::RvsdgDestruction,
                   "print-rvsdg-destruction",
                   "Write RVSDG destruction statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::RvsdgOptimization,
                   "print-rvsdg-optimization",
                   "Write RVSDG optimization statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::SteensgaardAnalysis,
                   "print-steensgaard-analysis",
                   "Write Steensgaard analysis statistics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::SteensgaardPointsToGraphConstruction,
                   "print-steensgaard-pointstograph-construction",
                   "Write Steensgaard PointsTo Graph construction statisitics to file."),
        clEnumValN(StatisticsDescriptor::StatisticsId::ThetaGammaInversion,
                   "print-ivt-stat",
                   "Write theta-gamma inversion statistics to file.")),
    cl::desc("Write statistics"));

	cl::opt<outputformat> format(
	  cl::values(
		  clEnumValN(outputformat::llvm, "llvm", "Output LLVM IR [default]")
		, clEnumValN(outputformat::xml, "xml", "Output XML"))
	, cl::desc("Select output format"));

  cl::list<jlm::OptimizationId> optids(
    cl::values(
      clEnumValN(
        jlm::OptimizationId::AASteensgaardBasic,
        "AASteensgaardBasic",
        "Steensgaard alias analysis with basic memory state encoding.")
      , clEnumValN(jlm::OptimizationId::cne, "cne", "Common node elimination")
      , clEnumValN(jlm::OptimizationId::dne, "dne", "Dead node elimination")
      , clEnumValN(jlm::OptimizationId::iln, "iln", "Function inlining"),
      clEnumValN(
        jlm::OptimizationId::InvariantValueRedirection,
        "InvariantValueRedirection",
        "Invariant Value Redirection")
      , clEnumValN(jlm::OptimizationId::psh, "psh", "Node push out")
      , clEnumValN(jlm::OptimizationId::pll, "pll", "Node pull in")
      , clEnumValN(jlm::OptimizationId::red, "red", "Node reductions")
      , clEnumValN(jlm::OptimizationId::ivt, "ivt", "Theta-gamma inversion")
      , clEnumValN(jlm::OptimizationId::url, "url", "Loop unrolling"))
    , cl::desc("Perform optimization"));

	cl::ParseCommandLineOptions(argc, argv);

	if (!ofile.empty())
		options.ofile = ofile;

	if (!sfile.empty())
		options.sd.set_file(sfile);

	std::vector<jlm::optimization*> optimizations;
	for (auto & optid : optids)
		optimizations.push_back(GetOptimization(optid));

  std::unordered_set<StatisticsDescriptor::StatisticsId> printStatisticsIds(
    printStatistics.begin(), printStatistics.end());

	options.ifile = ifile;
	options.format = format;
	options.optimizations = optimizations;
  options.sd.SetPrintStatisticsIds(printStatisticsIds);
}

}
