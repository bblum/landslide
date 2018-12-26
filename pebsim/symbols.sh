#!/bin/bash

# @file symbols.sh
# @brief Guide for landslide to know the names of pre-determined symbols during
#        the build process, which may change depending whether the kernel is
#        obfuscated, pintos, or neither.
# @author Ben Blum

# Copyright (c) 2018, Ben Blum
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived from
#       this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if [ "$OBFUSCATED_KERNEL" = 1 ]; then
	# Symbols needed by verify-tell in build.sh.
	TL_FORKING=esZREVcyfbPsQoxwXxGdcy
	TL_OFF_RQ=vhVvYAjGpsbRLTUgHVOKHOTKitUd
	TL_ON_RQ=ZlsuZyULncpscbkzngcjwKMxYTs
	TL_SWITCH=sDaRkslyaZLuPxZWEPYXSSIGflzC
	TL_INIT_DONE=QZUSpPBeeuyaxvNOTPDskqBkRLlYaz
	TL_VANISH=CHYodnfPjUDNzVkjLEwcuFJs
	# Other TLs (not needed by verify tell)
	TL_DECIDE=yfQWhIMzDSOkdDrpoHhACP
	TL_SLEEP=qbyLPGigbZcDIyutrNKAXfs
	TL_MX_LOCK=anaHvBljsftIyrnJvCvYaNhNLxMK
	TL_MX_BLOCK=wrARlvEveiMTtGngtOYrmbKugdBvB
	TL_MX_LOCK_DONE=hIHDtwffTBIXfpyDBDDqrnealQcvBqnLf
	TL_MX_UNLOCK=dTyptqDsHJWWhvWcaNDneYSsQPpwDl
	TL_MX_UNLOCK_DONE=pMAWEzfdDLmJNDpWmQmwnilwLiWozTHqBZs
	TL_MX_TRYLOCK=TeodbyzOXeMZbUQTAxtyRRYoVZSnjnX
	TL_MX_TRYLOCK_DONE=NCZHqnXwYHuxvDXpmmzVjUMGZkLqFRxsVsxV
	TL_STACK=VjxBuyTYEFQoSXIPEPggTeHcS
	# Other symbols needed by definegen.
	INIT_TSS=tDmmVTNO
	LMM_ALLOC=FtMkPemxx
	LMM_ALLOC_GEN=CZLoltqQauxoc
	LMM_FREE=RCFxRLuV
	LMM_INIT=MtqokdCaEfVRCwG
	LMM_ALLOC_SIZE_ARGNUM=2
	LMM_ALLOC_GEN_SIZE_ARGNUM=2
	LMM_FREE_BASE_ARGNUM=2
	KERN_PANIC=cnAky
	KERN_MAIN=wJTUxjBIOvQ
	KERN_HLT=yqI
	KERN_START=_start
else
	# Symbols needed by verify-tell in build.sh.
	TL_FORKING=tell_landslide_forking
	TL_OFF_RQ=tell_landslide_thread_off_rq
	TL_ON_RQ=tell_landslide_thread_on_rq
	TL_SWITCH=tell_landslide_thread_switch
	TL_INIT_DONE=tell_landslide_sched_init_done
	TL_VANISH=tell_landslide_vanishing
	# Other TLs (not needed by verify tell)
	TL_DECIDE=tell_landslide_preempt
	TL_SLEEP=tell_landslide_sleeping
	TL_MX_LOCK=tell_landslide_mutex_locking
	TL_MX_BLOCK=tell_landslide_mutex_blocking
	TL_MX_LOCK_DONE=tell_landslide_mutex_locking_done
	TL_MX_UNLOCK=tell_landslide_mutex_unlocking
	TL_MX_UNLOCK_DONE=tell_landslide_mutex_unlocking_done
	TL_MX_TRYLOCK=tell_landslide_mutex_trylocking
	TL_MX_TRYLOCK_DONE=tell_landslide_mutex_trylocking_done
	TL_STACK=tell_landslide_dump_stack
	# Other symbols needed by definegen.
	if [ -z "$PINTOS_KERNEL" ]; then
		# Pebbles.
		INIT_TSS=init_tss
		LMM_ALLOC=lmm_alloc
		LMM_ALLOC_GEN=lmm_alloc_gen
		LMM_FREE=lmm_free
		LMM_INIT=lmm_remove_free
		LMM_ALLOC_SIZE_ARGNUM=2
		LMM_ALLOC_GEN_SIZE_ARGNUM=2
		LMM_FREE_BASE_ARGNUM=2
		KERN_PANIC=panic
		KERN_MAIN=kernel_main
		KERN_HLT=hlt
		KERN_START=_start
	else
		# Pintos.
		KERN_HLT= # Idle hlts, but it's not in its own function.
		INIT_TSS= # FIXME - deferred to post userprog
		LMM_ALLOC=malloc
		LMM_ALLOC_GEN= # doesn't exists
		LMM_FREE=free
		LMM_INIT=malloc_init
		LMM_ALLOC_SIZE_ARGNUM=1
		LMM_ALLOC_GEN_SIZE_ARGNUM= # doesn't exists
		LMM_FREE_BASE_ARGNUM=1
		KERN_PANIC=debug_panic
		KERN_MAIN=main
		KERN_START=start
	fi
fi
