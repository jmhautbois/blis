/*

   BLIS    
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2014, The University of Texas

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    - Neither the name of The University of Texas nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "blis.h"

void bli_her2k_front( obj_t*  alpha,
                      obj_t*  a,
                      obj_t*  b,
                      obj_t*  beta,
                      obj_t*  c,
                      herk_t* cntl )
{
	obj_t    alpha_conj;
	obj_t    c_local;
	obj_t    a_local;
	obj_t    bh_local;
	obj_t    b_local;
	obj_t    ah_local;

	// Check parameters.
	if ( bli_error_checking_is_enabled() )
		bli_her2k_check( alpha, a, b, beta, c );

	// If alpha is zero, scale by beta and return.
	if ( bli_obj_equals( alpha, &BLIS_ZERO ) )
	{
		bli_scalm( beta, c );
		return;
	}

	// Alias A, B, and C in case we need to apply transformations.
	bli_obj_alias_to( *a, a_local );
	bli_obj_alias_to( *b, b_local );
	bli_obj_alias_to( *c, c_local );
	bli_obj_set_as_root( c_local );

	// For her2k, the first and second right-hand "B" operands are simply B'
	// and A'.
	bli_obj_alias_to( *b, bh_local );
	bli_obj_induce_trans( bh_local );
	bli_obj_toggle_conj( bh_local );
	bli_obj_alias_to( *a, ah_local );
	bli_obj_induce_trans( ah_local );
	bli_obj_toggle_conj( ah_local );

	// Initialize a conjugated copy of alpha.
	bli_obj_scalar_init_detached_copy_of( bli_obj_datatype( *a ),
	                                      BLIS_CONJUGATE,
	                                      alpha,
	                                      &alpha_conj );

	// An optimization: If C is row-stored, transpose the entire operation
	// so as to allow the macro-kernel more favorable access patterns
	// through C. (The effect of the transposition of A and A' is negligible
	// because those operands are always packed to contiguous memory.)
	if ( bli_obj_is_row_stored( c_local ) )
	{
		bli_obj_swap( a_local, bh_local );
		bli_obj_swap( b_local, ah_local );

		bli_obj_induce_trans( a_local );
		bli_obj_induce_trans( bh_local );
		bli_obj_induce_trans( b_local );
		bli_obj_induce_trans( ah_local );

		bli_obj_induce_trans( c_local );
	}

#if 0
	// Invoke the internal back-end.
	bli_her2k_int( alpha,
	               &a_local,
	               &bh_local,
	               &alpha_conj,
	               &b_local,
	               &ah_local,
	               beta,
	               &c_local,
	               cntl );
#else
	// Invoke herk twice, using beta only the first time.
	bli_herk_int( alpha,
	              &a_local,
	              &bh_local,
	              beta,
	              &c_local,
	              cntl );

	bli_herk_int( &alpha_conj,
	              &b_local,
	              &ah_local,
	              &BLIS_ONE,
	              &c_local,
	              cntl );
#endif
}

