/* This code is taken from libpcl library.
 * Rip-off done by stsp for dosemu2 project.
 * Original copyrights below. */

/*
 *  PCL by Davide Libenzi (Portable Coroutine Library)
 *  Copyright (C) 2003..2010  Davide Libenzi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "pcl_ctx.h"
#include "pcl.h"
#include "pcl_private.h"

static cothread_ctx *co_get_thread_ctx(void);

static int co_set_context(co_ctx_t *ctx, void *func, char *stkbase, long stksiz)
{
	if (GET_CTX(&ctx->cc))
		return -1;

	ctx->cc.uc_link = NULL;

	ctx->cc.uc_stack.ss_sp = stkbase;
	ctx->cc.uc_stack.ss_size = stksiz - sizeof(long);
	ctx->cc.uc_stack.ss_flags = 0;

	MAKE_CTX(&ctx->cc, func, 0);

	return 0;
}

static void co_switch_context(co_ctx_t *octx, co_ctx_t *nctx)
{
	cothread_ctx *tctx = co_get_thread_ctx();

	if (SWAP_CTX(&octx->cc, &nctx->cc) < 0) {
		fprintf(stderr, "[PCL] Context switch failed: curr=%p\n",
			tctx->co_curr);
		exit(1);
	}
}


static void co_runner(void)
{
	cothread_ctx *tctx = co_get_thread_ctx();
	coroutine *co = tctx->co_curr;

	co->restarget = co->caller;
	co->func(co->data);
	co_exit();
}

coroutine_t co_create(void (*func)(void *), void *data, void *stack, int size)
{
	int alloc = 0;
	coroutine *co;

	if ((size &= ~(sizeof(long) - 1)) < CO_MIN_SIZE)
		return NULL;
	if (stack == NULL) {
		size = (size + sizeof(coroutine) + CO_STK_ALIGN - 1) & ~(CO_STK_ALIGN - 1);
		stack = malloc(size);
		if (stack == NULL)
			return NULL;
		alloc = size;
	}
	co = stack;
	stack = (char *) stack + CO_STK_COROSIZE;
	co->alloc = alloc;
	co->func = func;
	co->data = data;
	co->exited = 0;
	if (co_set_context(&co->ctx, co_runner, stack, size - CO_STK_COROSIZE) < 0) {
		if (alloc)
			free(co);
		return NULL;
	}

	return (coroutine_t) co;
}

void co_delete(coroutine_t coro)
{
	cothread_ctx *tctx = co_get_thread_ctx();
	coroutine *co = (coroutine *) coro;

	if (co == tctx->co_curr) {
		fprintf(stderr, "[PCL] Cannot delete itself: curr=%p\n",
			tctx->co_curr);
		exit(1);
	}
	if (co->alloc)
		free(co);
}

void co_call(coroutine_t coro)
{
	cothread_ctx *tctx = co_get_thread_ctx();
	coroutine *co = (coroutine *) coro, *oldco = tctx->co_curr;

	co->caller = tctx->co_curr;
	tctx->co_curr = co;

	co_switch_context(&oldco->ctx, &co->ctx);

	if (co->exited)
		co_delete(co);
}

void co_resume(void)
{
	cothread_ctx *tctx = co_get_thread_ctx();

	co_call(tctx->co_curr->restarget);
	tctx->co_curr->restarget = tctx->co_curr->caller;
}

void co_exit(void)
{
	cothread_ctx *tctx = co_get_thread_ctx();
	coroutine *newco = tctx->co_curr->restarget, *co = tctx->co_curr;

	co->exited = 1;
	tctx->co_curr = newco;

	co_switch_context(&co->ctx, &newco->ctx);
}

coroutine_t co_current(void)
{
	cothread_ctx *tctx = co_get_thread_ctx();

	return (coroutine_t) tctx->co_curr;
}

void *co_get_data(coroutine_t coro)
{
	coroutine *co = (coroutine *) coro;

	return co->data;
}

void *co_set_data(coroutine_t coro, void *data)
{
	coroutine *co = (coroutine *) coro;
	void *odata;

	odata = co->data;
	co->data = data;

	return odata;
}

static cothread_ctx *co_get_global_ctx(void)
{
	static cothread_ctx tctx;

	if (tctx.co_curr == NULL)
		tctx.co_curr = &tctx.co_main;

	return &tctx;
}

/*
 * Simple case, the single thread one ...
 */

int co_thread_init(void)
{
	return 0;
}

void co_thread_cleanup(void)
{

}

static cothread_ctx *co_get_thread_ctx(void)
{
	return co_get_global_ctx();
}
