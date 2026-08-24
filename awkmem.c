/*
 * awkmem.c - memory management routines for gawk
 */

/*
 * Copyright (C) 2011-2015, 2017-2020, 2022, 2023, 2025, 2026
 * the Free Software Foundation, Inc.
 *
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 *
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "awk.h"

/*
 * For best performance, the INSTR_CHUNK value should be divisible by all
 * possible sizes, i.e. 1 through MAX_INSTRUCTION_ALLOC. Otherwise, there
 * will be wasted space at the end of the block.
 */
#define INSTR_CHUNK (2*3*4*7)

struct instruction_block {
	struct instruction_block *next;
	INSTRUCTION i[INSTR_CHUNK];
};

/* bcfree --- deallocate instruction */

void
bcfree(INSTRUCTION *cp)
{
	INSTRUCTION_POOL *pools = current_pools();	// get pools from execution context

	assert(cp->pool_size >= 1 && cp->pool_size <= MAX_INSTRUCTION_ALLOC);

	cp->opcode = Op_illegal;
	cp->nexti = pools->pool[cp->pool_size - 1].free_list;
	pools->pool[cp->pool_size - 1].free_list = cp;
}

/* bcalloc --- allocate a new instruction */

INSTRUCTION *
bcalloc(OPCODE op, int size, int srcline)
{
	INSTRUCTION *cp;
	INSTRUCTION_POOL *pools = current_pools();	// get pools from execution context
	struct instruction_mem_pool *pool;

	assert(size >= 1 && size <= MAX_INSTRUCTION_ALLOC);
	pool = &pools->pool[size - 1];

	if (pool->free_list != NULL) {
		cp = pool->free_list;
		pool->free_list = cp->nexti;
	} else if (pool->free_space && pool->free_space + size <= & pool->block_list->i[INSTR_CHUNK]) {
		cp = pool->free_space;
		pool->free_space += size;
	} else {
		struct instruction_block *block;
		emalloc(block, struct instruction_block *, sizeof(struct instruction_block));
		block->next = pool->block_list;
		pool->block_list = block;
		cp = &block->i[0];
		pool->free_space = &block->i[size];
	}

	memset(cp, 0, size * sizeof(INSTRUCTION));
	cp->pool_size = size;
	cp->opcode = op;
	cp->source_line = srcline;
	return cp;
}

/* free_bc_internal --- free internal memory of an instruction. */

static void
free_bc_internal(INSTRUCTION *cp)
{
	NODE *m;

	switch(cp->opcode) {
	case Op_func_call:
		if (cp->func_name != NULL)
			efree(cp->func_name);
		break;
	case Op_push_re:
	case Op_match_rec:
	case Op_match:
	case Op_nomatch:
		m = cp->memory;
		if (m->re_reg[0] != NULL)
			refree(m->re_reg[0]);
		if (m->re_reg[1] != NULL)
			refree(m->re_reg[1]);
		if (m->re_exp != NULL)
			unref(m->re_exp);
		if (m->re_text != NULL)
			unref(m->re_text);
		freenode(m);
		break;
	case Op_token:
		/* token lost during error recovery in yyparse */
		if (cp->lextok != NULL)
			efree(cp->lextok);
		break;
	case Op_push_i:
		m = cp->memory;
		unref(m);
		break;
	case Op_store_var:
		m = cp->initval;
		if (m != NULL)
			unref(m);
		break;
	case Op_illegal:
		cant_happen("unexpected opcode %s", opcode2str(cp->opcode));
	default:
		break;
	}
}

/* free_bc_mempool --- free a single pool */

static void
free_bc_mempool(struct instruction_mem_pool *pool, int size)
{
	bool first = true;
	struct instruction_block *block, *next;

	for (block = pool->block_list; block; block = next) {
		INSTRUCTION *cp, *end;

		end = (first ? pool->free_space : & block->i[INSTR_CHUNK]);
		for (cp = & block->i[0]; cp + size <= end; cp += size) {
			if (cp->opcode != Op_illegal)
				free_bc_internal(cp);
		}
		next = block->next;
		efree(block);
		first = false;
	}
}


/* free_bcpool --- free list of instruction memory pools */

void
free_bcpool(INSTRUCTION_POOL *pl)
{
	int i;

	for (i = 0; i < MAX_INSTRUCTION_ALLOC; i++)
		free_bc_mempool(& pl->pool[i], i + 1);
}
