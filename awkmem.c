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
			free(cp->func_name);
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
			free(cp->lextok);
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
		// Do nothing! Most opcodes don't have anything
		// special that needs to be done upon being released.
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
		free(block);
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

#define BLOCKCHUNK 100

struct block_header nextfree[BLOCK_MAX] = {
	{ NULL, sizeof(NODE), "node" },
	{ NULL, sizeof(BUCKET), "bucket" },
};

#ifdef MEMDEBUG

void *
r_getblock(int id)
{
	void *res;
	ezalloc(res, void *, nextfree[id].size);
	nextfree[id].active++;
	if (nextfree[id].highwater < nextfree[id].active)
		nextfree[id].highwater = nextfree[id].active;
	return res;
}

void
r_freeblock(void *p, int id)
{
#ifdef HAVE_MPFR
	NODE *n = (NODE *) p;

	if (id == BLOCK_NODE && n->mpfr_data != NULL) {
		free(n->mpfr_data);
		n->mpfr_data = NULL;
	}
#endif
	nextfree[id].active--;
	free(p);
}

#else

static struct block_item *more_blocks(int id);

void *
r_getblock(int id)
{
	struct block_item *bp = nextfree[id].freep;
	if (bp != NULL)
		nextfree[id].freep = bp->freep;
	else
		bp = more_blocks(id);

	memset(bp, '\0', nextfree[id].size);
	return bp;
}

void
r_freeblock(void *p, int id)
{
	struct block_item *bp = (struct block_item *) p;

#ifdef HAVE_MPFR
	NODE *n = (NODE *) p;

	if (id == BLOCK_NODE && n->mpfr_data != NULL) {
		free(n->mpfr_data);
		n->mpfr_data = NULL;
	}
#endif
	bp->freep = nextfree[id].freep;
	nextfree[id].freep = bp;
}


/* 
 * more_blocks --- get more blocks of memory and add to the free list;
 * size of a block must be >= sizeof(struct block_item)
 */

static struct block_item *
more_blocks(int id)
{
	struct block_item *freep, *np, *next;
	char *p, *endp;
	size_t size;

	size = nextfree[id].size;

	assert(size >= sizeof(struct block_item));
	emalloc(freep, struct block_item *, BLOCKCHUNK * size);
	p = (char *) freep;
	endp = p + BLOCKCHUNK * size;

	for (np = freep; ; np = next) {
		next = (struct block_item *) (p += size);
		if (p >= endp) {
			np->freep = NULL;
			break;
		}
		np->freep = next;
	}
	nextfree[id].freep = freep->freep;
	nextfree[id].highwater += BLOCKCHUNK;
	return freep;
}

#endif
