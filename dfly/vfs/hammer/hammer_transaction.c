/*
 * Copyright (c) 2007-2008 The DragonFly Project.  All rights reserved.
 * 
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * $DragonFly: src/sys/vfs/hammer/hammer_transaction.c,v 1.25 2008/09/23 21:03:52 dillon Exp $
 */

#include "hammer.h"

static hammer_tid_t hammer_alloc_tid(hammer_mount_t hmp, int count);


/*
 * Start a standard transaction.
 */
void
hammer_start_transaction(struct hammer_transaction *trans,
			 struct hammer_mount *hmp)
{
	struct timeval tv;
	int error;

	trans->type = HAMMER_TRANS_STD;
	trans->hmp = hmp;
	trans->rootvol = hammer_get_root_volume(hmp, &error);
	KKASSERT(error == 0);
	trans->tid = 0;
	trans->sync_lock_refs = 0;
	trans->flags = 0;

	getmicrotime(&tv);
	trans->time = (unsigned long)tv.tv_sec * 1000000ULL + tv.tv_usec;
	trans->time32 = (u_int32_t)tv.tv_sec;
}

/*
 * Start a simple read-only transaction.  This will not stall.
 */
void
hammer_simple_transaction(struct hammer_transaction *trans,
			  struct hammer_mount *hmp)
{
	struct timeval tv;
	int error;

	trans->type = HAMMER_TRANS_RO;
	trans->hmp = hmp;
	trans->rootvol = hammer_get_root_volume(hmp, &error);
	KKASSERT(error == 0);
	trans->tid = 0;
	trans->sync_lock_refs = 0;
	trans->flags = 0;

	getmicrotime(&tv);
	trans->time = (unsigned long)tv.tv_sec * 1000000ULL + tv.tv_usec;
	trans->time32 = (u_int32_t)tv.tv_sec;
}

/*
 * Start a transaction using a particular TID.  Used by the sync code.
 * This does not stall.
 *
 * This routine may only be called from the flusher thread.  We predispose
 * sync_lock_refs, implying serialization against the synchronization stage
 * (which the flusher is responsible for).
 */
void
hammer_start_transaction_fls(struct hammer_transaction *trans,
			     struct hammer_mount *hmp)
{
	struct timeval tv;
	int error;

	bzero(trans, sizeof(*trans));

	trans->type = HAMMER_TRANS_FLS;
	trans->hmp = hmp;
	trans->rootvol = hammer_get_root_volume(hmp, &error);
	KKASSERT(error == 0);
	trans->tid = hammer_alloc_tid(hmp, 1);
	trans->sync_lock_refs = 1;
	trans->flags = 0;

	getmicrotime(&tv);
	trans->time = (unsigned long)tv.tv_sec * 1000000ULL + tv.tv_usec;
	trans->time32 = (u_int32_t)tv.tv_sec;
}

void
hammer_done_transaction(struct hammer_transaction *trans)
{
	hammer_mount_t hmp = trans->hmp;
	int expected_lock_refs;

	hammer_rel_volume(trans->rootvol, 0);
	trans->rootvol = NULL;
	expected_lock_refs = (trans->type == HAMMER_TRANS_FLS) ? 1 : 0;
	KKASSERT(trans->sync_lock_refs == expected_lock_refs);
	trans->sync_lock_refs = 0;
	if (trans->type != HAMMER_TRANS_FLS) {
		if (trans->flags & HAMMER_TRANSF_NEWINODE)
			hammer_inode_waitreclaims(hmp);
		else if (trans->flags & HAMMER_TRANSF_DIDIO)
			hammer_inode_waithard(hmp);
	}
}

/*
 * Allocate (count) TIDs.  If running in multi-master mode the returned
 * base will be aligned to a 16-count plus the master id (0-15).  
 * Multi-master mode allows non-conflicting to run and new objects to be
 * created on multiple masters in parallel.  The transaction id identifies
 * the original master.  The object_id is also subject to this rule in
 * order to allow objects to be created on multiple masters in parallel.
 *
 * Directories may pre-allocate a large number of object ids (100,000).
 *
 * NOTE: There is no longer a requirement that successive transaction
 * ids be 2 apart for separator generation.
 */
static hammer_tid_t
hammer_alloc_tid(hammer_mount_t hmp, int count)
{
	hammer_tid_t tid;

	if (hmp->master_id < 0) {
		tid = hmp->next_tid + 1;
		hmp->next_tid = tid + count;
	} else {
		tid = (hmp->next_tid + HAMMER_MAX_MASTERS) &
		      ~(hammer_tid_t)(HAMMER_MAX_MASTERS - 1);
		hmp->next_tid = tid + count * HAMMER_MAX_MASTERS;
		tid |= hmp->master_id;
	}
	if (tid >= 0xFFFFFFFFFF000000ULL)
		panic("hammer_start_transaction: Ran out of TIDs!");
	if (hammer_debug_tid)
		kprintf("alloc_tid %016llx\n", tid);
	return(tid);
}

/*
 * Allocate an object id
 */
hammer_tid_t
hammer_alloc_objid(hammer_mount_t hmp, hammer_inode_t dip)
{
	hammer_objid_cache_t ocp;
	hammer_tid_t tid;

	while ((ocp = dip->objid_cache) == NULL) {
		if (hmp->objid_cache_count < OBJID_CACHE_SIZE) {
			ocp = kmalloc(sizeof(*ocp), hmp->m_misc,
				      M_WAITOK|M_ZERO);
			ocp->next_tid = hammer_alloc_tid(hmp, OBJID_CACHE_BULK);
			ocp->count = OBJID_CACHE_BULK;
			TAILQ_INSERT_HEAD(&hmp->objid_cache_list, ocp, entry);
			++hmp->objid_cache_count;
			/* may have blocked, recheck */
			if (dip->objid_cache == NULL) {
				dip->objid_cache = ocp;
				ocp->dip = dip;
			}
		} else {
			ocp = TAILQ_FIRST(&hmp->objid_cache_list);
			if (ocp->dip)
				ocp->dip->objid_cache = NULL;
			dip->objid_cache = ocp;
			ocp->dip = dip;
		}
	}
	TAILQ_REMOVE(&hmp->objid_cache_list, ocp, entry);

	/*
	 * The TID is incremented by 1 or by 16 depending what mode the
	 * mount is operating in.
	 */
	tid = ocp->next_tid;
	ocp->next_tid += (hmp->master_id < 0) ? 1 : HAMMER_MAX_MASTERS;

	if (--ocp->count == 0) {
		dip->objid_cache = NULL;
		--hmp->objid_cache_count;
		ocp->dip = NULL;
		kfree(ocp, hmp->m_misc);
	} else {
		TAILQ_INSERT_TAIL(&hmp->objid_cache_list, ocp, entry);
	}
	return(tid);
}

void
hammer_clear_objid(hammer_inode_t dip)
{
	hammer_objid_cache_t ocp;

	if ((ocp = dip->objid_cache) != NULL) {
		dip->objid_cache = NULL;
		ocp->dip = NULL;
		TAILQ_REMOVE(&dip->hmp->objid_cache_list, ocp, entry);
		TAILQ_INSERT_HEAD(&dip->hmp->objid_cache_list, ocp, entry);
	}
}

void
hammer_destroy_objid_cache(hammer_mount_t hmp)
{
	hammer_objid_cache_t ocp;

	while ((ocp = TAILQ_FIRST(&hmp->objid_cache_list)) != NULL) {
		TAILQ_REMOVE(&hmp->objid_cache_list, ocp, entry);
		if (ocp->dip)
			ocp->dip->objid_cache = NULL;
		kfree(ocp, hmp->m_misc);
	}
}

