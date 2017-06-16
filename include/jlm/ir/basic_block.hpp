/*
 * Copyright 2013 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_IR_BASIC_BLOCK_H
#define JLM_IR_BASIC_BLOCK_H

#include <jlm/ir/cfg.hpp>
#include <jlm/ir/cfg_node.hpp>
#include <jlm/ir/tac.hpp>

namespace jive {
	class operation;
}

namespace jlm {

class expr;

class basic_block final : public attribute {
	typedef std::list<const tac*>::const_iterator const_iterator;
	typedef std::list<const tac*>::const_reverse_iterator const_reverse_iterator;
public:
	virtual
	~basic_block();

	inline
	basic_block()
	: attribute()
	{}

	inline
	basic_block(const basic_block & other)
	: attribute(other)
	{
		for (auto & tac : other.tacs_)
			tacs_.push_back(new jlm::tac(*tac));
	}

	basic_block(basic_block &&) = delete;

	inline basic_block &
	operator=(const basic_block & other)
	{
		if (this == &other)
			return *this;

		for (const auto & tac : other.tacs_)
			delete tac;

		tacs_.clear();
		for (const auto & tac : other.tacs_)
			tacs_.push_back(new jlm::tac(*tac));

		return *this;
	}

	basic_block &
	operator=(basic_block &&) = delete;

	inline const_iterator
	begin() const noexcept
	{
		return tacs_.begin();
	}

	inline const_reverse_iterator
	rbegin() const noexcept
	{
		return tacs_.rbegin();
	}

	inline const_iterator
	end() const noexcept
	{
		return tacs_.end();
	}

	inline const_reverse_iterator
	rend() const noexcept
	{
		return tacs_.rend();
	}

	inline const tac *
	append(std::unique_ptr<jlm::tac> tac)
	{
		tacs_.push_back(tac.release());
		return tacs_.back();
	}

	inline void
	append(std::vector<std::unique_ptr<jlm::tac>> & tacs)
	{
		for (auto & tac : tacs)
			tacs_.push_back(tac.release());
	}

	inline size_t
	ntacs() const noexcept
	{
		return tacs_.size();
	}

	inline const tac *
	first() const noexcept
	{
		return ntacs() != 0 ? tacs_.front() : nullptr;
	}

	inline const tac *
	last() const noexcept
	{
		return ntacs() != 0 ? tacs_.back() : nullptr;
	}

	inline void
	drop_first()
	{
		tacs_.pop_front();
	}

	inline void
	drop_last()
	{
		tacs_.pop_back();
	}

	virtual std::string
	debug_string() const noexcept override;

	virtual std::unique_ptr<attribute>
	copy() const override;

private:
	std::list<const tac*> tacs_;
};

static inline cfg_node *
create_basic_block_node(jlm::cfg * cfg)
{
	basic_block attr;
	return cfg->create_node(attr);
}

static inline basic_block *
create_basic_block(jlm::cfg * cfg)
{
	return static_cast<basic_block*>(&create_basic_block_node(cfg)->attribute());
}

static inline bool
is_basic_block(const jlm::cfg_node * node) noexcept
{
	return dynamic_cast<const basic_block*>(&node->attribute()) != nullptr;
}

}

#endif