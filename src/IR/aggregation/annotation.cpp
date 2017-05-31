/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/IR/aggregation/annotation.hpp>
#include <jlm/IR/aggregation/node.hpp>
#include <jlm/IR/basic_block.hpp>

#include <algorithm>

namespace jlm {
namespace agg {

static void
annotate(const agg::node * node, dset & pds, demand_map & dm);

static inline std::unique_ptr<demand_set>
annotate_basic_block(const basic_block & bb, dset & pds)
{
	auto ds = create_demand_set(pds);
	for (auto it = bb.rbegin(); it != bb.rend(); it++) {
		for (size_t n = 0; n < (*it)->noutputs(); n++)
			pds.erase((*it)->output(n));
		for (size_t n = 0; n < (*it)->ninputs(); n++)
			pds.insert((*it)->input(n));
	}
	ds->top = pds;

	return ds;
}

static inline void
annotate_entry(const agg::node * node, dset & pds, demand_map & dm)
{
	JLM_DEBUG_ASSERT(is_entry_structure(node->structure()));
	const auto & ea = static_cast<const entry*>(&node->structure())->attribute();

	auto ds = create_demand_set(pds);
	for (size_t n = 0; n < ea.narguments(); n++)
		pds.erase(ea.argument(n));

	ds->top = pds;
	JLM_DEBUG_ASSERT(dm.find(node) == dm.end());
	dm[node] = std::move(ds);
}

static inline void
annotate_exit(const agg::node * node, dset & pds, demand_map & dm)
{
	JLM_DEBUG_ASSERT(is_exit_structure(node->structure()));
	const auto & xa = static_cast<const exit*>(&node->structure())->attribute();

	auto ds = create_demand_set(pds);
	for (size_t n = 0; n < xa.nresults(); n++)
		pds.insert(xa.result(n));

	ds->top = pds;
	JLM_DEBUG_ASSERT(dm.find(node) == dm.end());
	dm[node] = std::move(ds);
}

static inline void
annotate_block(const agg::node * node, dset & pds, demand_map & dm)
{
	JLM_DEBUG_ASSERT(is_block_structure(node->structure()));
	const auto & bb = static_cast<const block*>(&node->structure())->basic_block();

	JLM_DEBUG_ASSERT(dm.find(node) == dm.end());
	dm[node] = annotate_basic_block(bb, pds);
}

static inline void
annotate_linear(const agg::node * node, dset & pds, demand_map & dm)
{
	JLM_DEBUG_ASSERT(is_linear_structure(node->structure()));

	auto ds = create_demand_set(pds);
	for (ssize_t n = node->nchildren()-1; n >= 0; n--)
		annotate(node->child(n), pds, dm);
	ds->top = pds;

	JLM_DEBUG_ASSERT(dm.find(node) == dm.end());
	dm[node] = std::move(ds);
}

static inline void
annotate_branch(const agg::node * node, dset & pds, demand_map & dm)
{
	JLM_DEBUG_ASSERT(is_branch_structure(node->structure()));
	const auto & branch = static_cast<const jlm::agg::branch*>(&node->structure());

	auto ds = create_branch_demand_set(pds);
	annotate_basic_block(branch->join(), pds);

	dset cases_top;
	ds->cases_bottom = pds;
	for (size_t n = 0; n < node->nchildren(); n++) {
		auto tmp = pds;
		annotate(node->child(n), tmp, dm);
		cases_top.insert(tmp.begin(), tmp.end());
	}
	ds->cases_top = pds = cases_top;

	annotate_basic_block(branch->split(), pds);
	ds->top = pds;

	JLM_DEBUG_ASSERT(dm.find(node) == dm.end());
	dm[node] = std::move(ds);
}

static inline void
annotate_loop(const agg::node * node, dset & pds, demand_map & dm)
{
	JLM_DEBUG_ASSERT(is_loop_structure(node->structure()));
	JLM_DEBUG_ASSERT(node->nchildren() == 1);

	auto ds = create_demand_set(pds);
	annotate(node->child(0), pds, dm);
	if (ds->bottom != pds) {
		ds->top = pds;
		annotate(node->child(0), pds, dm);
		JLM_DEBUG_ASSERT(ds->top == pds);
	}
	ds->bottom = ds->top;

	JLM_DEBUG_ASSERT(dm.find(node) == dm.end());
	dm[node] = std::move(ds);
}

static inline void
annotate(const agg::node * node, dset & pds, demand_map & dm)
{
	static std::unordered_map<
		std::type_index,
		std::function<void(const agg::node*, dset&, demand_map&)>
	> map({
	  {std::type_index(typeid(entry)), annotate_entry}
	, {std::type_index(typeid(exit)), annotate_exit}
	, {std::type_index(typeid(block)), annotate_block}
	, {std::type_index(typeid(linear)), annotate_linear}
	, {std::type_index(typeid(branch)), annotate_branch}
	, {std::type_index(typeid(loop)), annotate_loop}
	});

	auto it = dm.find(node);
	if (it != dm.end() && it->second->top == pds)
		return;

	JLM_DEBUG_ASSERT(map.find(std::type_index(typeid(node->structure()))) != map.end());
	return map[std::type_index(typeid(node->structure()))](node, pds, dm);
}

demand_map
annotate(jlm::agg::node & root)
{
	dset ds;
	demand_map dm;
	annotate(&root, ds, dm);
	return dm;
}

}}