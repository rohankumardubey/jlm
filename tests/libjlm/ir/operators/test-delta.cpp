/*
 * Copyright 2018 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <test-operation.hpp>
#include <test-types.hpp>
#include <test-registry.hpp>

#include <jive/view.h>

#include <jlm/jlm/ir/rvsdg.hpp>
#include <jlm/jlm/ir/operators/delta.hpp>

static int
test()
{
	using namespace jlm;

	/* setup graph */

	valuetype vt;
	jlm::rvsdg rvsdg("", "");

	auto imp = rvsdg.graph()->add_import(vt, "");

	delta_builder db;
	db.begin(rvsdg.graph()->root(), vt, linkage::external_linkage, true);
	auto dep = db.add_dependency(imp);
	auto d1 = db.end(create_testop(db.region(), {dep}, {&vt})[0]);

	db.begin(rvsdg.graph()->root(), vt, linkage::internal_linkage, false);
	auto d2 = db.end(create_testop(db.region(), {}, {&vt})[0]);

	rvsdg.graph()->add_export(d1, "");
	rvsdg.graph()->add_export(d2, "");

	jive::view(*rvsdg.graph(), stdout);

	/* verify graph */

	assert(rvsdg.graph()->root()->nnodes() == 2);

	auto delta1 = static_cast<const delta_node*>(d1->node());
	assert(delta1->linkage() == linkage::external_linkage);
	assert(delta1->constant() == true);
	assert(delta1->valuetype() == vt);

	auto delta2= static_cast<const delta_node*>(d2->node());
	assert(delta2->linkage() == linkage::internal_linkage);
	assert(delta2->constant() == false);
	assert(delta2->valuetype() == vt);

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/ir/operators/test-delta", test)
