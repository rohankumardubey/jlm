/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/common.hpp>
#include <jlm/ir/data.hpp>
#include <jlm/opt/dne.hpp>

#include <jive/types/function/fctlambda.h>
#include <jive/vsdg/gamma.h>
#include <jive/vsdg/phi.h>
#include <jive/vsdg/simple_node.h>
#include <jive/vsdg/structural_node.h>
#include <jive/vsdg/theta.h>
#include <jive/vsdg/traverser.h>

namespace jlm {

class dnectx {
public:
	inline void
	mark(const jive::input * input)
	{
		inputs_.insert(input);
	}

	inline void
	mark(const jive::node * node)
	{
		for (size_t n = 0; n < node->ninputs(); n++)
			mark(node->input(n));
	}

	inline bool
	is_dead(const jive::input * input) const noexcept
	{
		return inputs_.find(input) != inputs_.end();
	}

	inline bool
	is_dead(const jive::output * output) const noexcept
	{
		if (output->nusers() == 0)
			return true;

		for (const auto & user : *output) {
			if (!is_dead(user))
				return false;
		}

		return true;
	}

private:
	std::unordered_set<const jive::input*> inputs_;
};

/* mark phase */

static void
mark(jive::region*, dnectx&);

static void
mark_data(const jive::structural_node * node, dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const data_op*>(&node->operation()));

	if (ctx.is_dead(node->output(0)))
		ctx.mark(node);
}

static void
mark_phi(const jive::structural_node * node, dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::phi_op*>(&node->operation()));
	auto subregion = node->subregion(0);

	/* mark functions */
	for (size_t n = 0; n < subregion->nresults(); n++) {
		if (ctx.is_dead(subregion->result(n)->output()))
			ctx.mark(subregion->result(n));
	}

	mark(subregion, ctx);

	/* mark dependencies */
	for (size_t n = 0; n < subregion->narguments(); n++) {
		if (subregion->argument(n)->input() == nullptr)
			continue;

		if (ctx.is_dead(subregion->argument(n)))
			ctx.mark(subregion->argument(n)->input());
	}
}

static void
mark_lambda(const jive::structural_node * node, dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::fct::lambda_op*>(&node->operation()));
	auto subregion = node->subregion(0);

	mark(subregion, ctx);

	/* mark context variables as dead */
	for (size_t n = 0; n < subregion->narguments(); n++) {
		auto argument = subregion->argument(n);
		if (argument->input() == nullptr)
			continue;

		if (ctx.is_dead(argument))
			ctx.mark(argument->input());
	}
}

static void
mark_theta(const jive::structural_node * node, dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::theta_op*>(&node->operation()));

	/* mark loops exits */
	for (size_t n = 0; n < node->noutputs(); n++) {
		if (ctx.is_dead(node->output(n)))
			ctx.mark(node->subregion(0)->result(n+1));
	}

	mark(node->subregion(0), ctx);

	/* mark loop entries */
	auto subregion = node->subregion(0);
	for (size_t n = 0; n < subregion->narguments(); n++) {
		if (ctx.is_dead(subregion->argument(n)))
			ctx.mark(subregion->argument(n)->input());
	}
}

static void
mark_gamma(const jive::structural_node * node, dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::gamma_op*>(&node->operation()));

	/* mark exit variables */
	for (size_t n = 0; n < node->noutputs(); n++) {
		if (ctx.is_dead(node->output(n))) {
			for (size_t r = 0; r < node->nsubregions(); r++)
				ctx.mark(node->subregion(r)->result(n));
		}
	}

	for (size_t n = 0; n < node->nsubregions(); n++)
		mark(node->subregion(n), ctx);

	/* mark entry variables */
	for (size_t n = 1; n < node->ninputs(); n++) {
		size_t r;
		for (r = 0; r < node->nsubregions(); r++) {
			if (!ctx.is_dead(node->subregion(r)->argument(n-1)))
				break;
		}

		if (r == node->nsubregions())
			ctx.mark(node->input(n));
	}
}

static void
mark(const jive::structural_node * node, dnectx & ctx)
{
	static std::unordered_map<
		std::type_index
	, void(*)(const jive::structural_node*, dnectx&)
	> map({
	  {std::type_index(typeid(jive::gamma_op)), mark_gamma}
	, {std::type_index(typeid(jive::theta_op)), mark_theta}
	, {std::type_index(typeid(jive::fct::lambda_op)), mark_lambda}
	, {std::type_index(typeid(jive::phi_op)), mark_phi}
	, {std::type_index(typeid(jlm::data_op)), mark_data}
	});

	std::type_index index(typeid(node->operation()));
	JLM_DEBUG_ASSERT(map.find(index) != map.end());
	map[index](node, ctx);
}

static void
mark(const jive::simple_node * node, dnectx & ctx)
{
	for (size_t n = 0; n < node->noutputs(); n++) {
		if (!ctx.is_dead(node->output(n)))
			return;
	}

	ctx.mark(node);
}

static void
mark(jive::region * region, dnectx & ctx)
{
	for (const auto & node : jive::bottomup_traverser(region)) {
		if (auto simple = dynamic_cast<const jive::simple_node*>(node))
			mark(simple, ctx);
		else
			mark(static_cast<const jive::structural_node*>(node), ctx);
	}
}

/* sweep phase */

static void
sweep(jive::region * region, const dnectx & ctx);

static void
sweep_data(jive::structural_node * node, const dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const data_op*>(&node->operation()));

	if (node->output(0)->nusers() == 0) {
		node->region()->remove_node(node);
		return;
	}

	sweep(node->subregion(0), ctx);
}

static void
sweep_phi(jive::structural_node * node, const dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::phi_op*>(&node->operation()));
	auto subregion = node->subregion(0);

	/* remove outputs and results */
	std::vector<jive::argument*> dead_arguments;
	for (ssize_t n = subregion->nresults()-1; n >= 1; n--) {
		auto result = subregion->result(n);
		if (ctx.is_dead(result)
		&& ctx.is_dead(subregion->argument(result->index()))) {
			dead_arguments.push_back(subregion->argument(result->index()));
			subregion->remove_result(n);
			node->remove_output(n);
		}
	}

	if (node->noutputs() == 0) {
		node->region()->remove_node(node);
		return;
	}

	sweep(subregion, ctx);

	/* remove dead function arguments */
	for (const auto & argument : dead_arguments) {
		JLM_DEBUG_ASSERT(argument->input() == nullptr);
		subregion->remove_argument(argument->index());
	}

	/* remove dead dependencies */
	for (ssize_t n = subregion->narguments()-1; n >= 0; n--) {
		auto argument = subregion->argument(n);
		if (argument->nusers() == 0 && argument->input()) {
				size_t index = argument->input()->index();
				subregion->remove_argument(n);
				node->remove_input(index);
		}
	}
}

static void
sweep_lambda(jive::structural_node * node, const dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::fct::lambda_op*>(&node->operation()));
	auto subregion = node->subregion(0);

	if (!node->has_users()) {
		remove(node);
		return;
	}

	sweep(subregion, ctx);

	/* remove inputs and arguments */
	for (ssize_t n = subregion->narguments()-1; n >= 0; n--) {
		auto argument = subregion->argument(n);
		if (argument->input() == nullptr)
			continue;

		if (argument->nusers() == 0) {
			size_t index = argument->input()->index();
			subregion->remove_argument(n);
			node->remove_input(index);
		}
	}
}

static void
sweep_theta(jive::structural_node * node, const dnectx & ctx)
{
	JLM_DEBUG_ASSERT(is_theta_op(node->operation()));

	if (!node->has_users()) {
		remove(node);
		return;
	}

	/* remove results */
	auto subregion = node->subregion(0);
	for (ssize_t n = subregion->nresults()-1; n >= 1; n--) {
		auto result = subregion->result(n);
		if (ctx.is_dead(result->output()) && ctx.is_dead(node->input(n-1)))
			subregion->remove_result(n);
	}

	sweep(subregion, ctx);

	/* remove outputs, inputs, and arguments */
	for (ssize_t n = subregion->narguments()-1; n >= 0; n--) {
		auto argument = subregion->argument(n);
		if (ctx.is_dead(node->output(n)) && ctx.is_dead(argument->input())) {
			subregion->remove_argument(n);
			node->remove_input(n);
			node->remove_output(n);
		}
	}

	JLM_DEBUG_ASSERT(node->ninputs() == node->noutputs());
}

static void
sweep_gamma(jive::structural_node * node, const dnectx & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::gamma_op*>(&node->operation()));

	/* remove results */
	for (size_t r = 0; r < node->nsubregions(); r++) {
		auto subregion = node->subregion(r);
		for (ssize_t n = subregion->nresults()-1; n >= 0; n--) {
			if (subregion->result(n)->output()->nusers() == 0)
				subregion->remove_result(n);
		}
	}

	/* remove outputs */
	for (ssize_t n = node->noutputs()-1; n >= 0; n--) {
		if (node->output(n)->nusers() == 0)
			node->remove_output(n);
	}

	if (node->noutputs() == 0) {
		node->region()->remove_node(node);
		return;
	}

	for (size_t r = 0; r < node->nsubregions(); r++)
		sweep(node->subregion(r), ctx);

	/* remove arguments and inputs */
	for (ssize_t n = node->ninputs()-1; n >=  1; n--) {
		auto input = node->input(n);
		jive::argument * argument;
		JIVE_LIST_ITERATE(input->arguments, argument, input_argument_list) {
			if (argument->nusers() != 0)
				break;
		}

		if (argument == nullptr) {
			for (size_t r = 0; r < node->nsubregions(); r++)
				node->subregion(r)->remove_argument(n-1);
			node->remove_input(n);
		}
	}
}

static void
sweep(jive::structural_node * node, const dnectx & ctx)
{
	static std::unordered_map<
		std::type_index
	, void(*)(jive::structural_node*, const dnectx&)
	> map({
	  {std::type_index(typeid(jive::gamma_op)), sweep_gamma}
	, {std::type_index(typeid(jive::theta_op)), sweep_theta}
	, {std::type_index(typeid(jive::fct::lambda_op)), sweep_lambda}
	, {std::type_index(typeid(jive::phi_op)), sweep_phi}
	, {std::type_index(typeid(jlm::data_op)), sweep_data}
	});

	std::type_index index(typeid(node->operation()));
	JLM_DEBUG_ASSERT(map.find(index) != map.end());
	map[index](node, ctx);
}

static void
sweep(jive::simple_node * node, const dnectx & ctx)
{
	for (size_t n = 0; n < node->noutputs(); n++) {
		if (node->output(n)->nusers() != 0)
			return;
	}

	node->region()->remove_node(node);
}

static void
sweep(jive::region * region, const dnectx & ctx)
{
	for (const auto & node : jive::bottomup_traverser(region)) {
		if (auto simple = dynamic_cast<jive::simple_node*>(node))
			sweep(simple, ctx);
		else
			sweep(static_cast<jive::structural_node*>(node), ctx);
	}
	JLM_DEBUG_ASSERT(region->bottom_nodes.first == nullptr);
}

void
dne(jive::graph & graph)
{
	auto root = graph.root();

	dnectx ctx;
	mark(root, ctx);
	sweep(root, ctx);

	for (ssize_t n = root->narguments()-1; n >= 0; n--) {
		auto argument = root->argument(n);
		if (argument->nusers() == 0) {
			root->remove_argument(n);
		}
	}
}

}
