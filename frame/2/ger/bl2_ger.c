/*

   BLIS    
   An object-based framework for developing high-performance BLAS-like
   libraries.

   Copyright (C) 2012, The University of Texas

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

#include "blis2.h"

extern ger_t* ger_cntl_bs_ke_row;
extern ger_t* ger_cntl_bs_ke_col;
extern ger_t* ger_cntl_ge_row;
extern ger_t* ger_cntl_ge_col;

void bl2_ger( obj_t*  alpha,
              obj_t*  x,
              obj_t*  y,
              obj_t*  a )
{
	ger_t*  ger_cntl;
	num_t   dt_targ_x;
	num_t   dt_targ_y;
	num_t   dt_targ_a;
	bool_t  x_is_contig;
	bool_t  y_is_contig;
	bool_t  a_is_contig;
	obj_t   alpha_local;
	num_t   dt_alpha;

	// Check parameters.
	if ( bl2_error_checking_is_enabled() )
		bl2_ger_check( alpha, x, y, a );


	// Query the target datatypes of each object.
	dt_targ_x = bl2_obj_target_datatype( *x );
	dt_targ_y = bl2_obj_target_datatype( *y );
	dt_targ_a = bl2_obj_target_datatype( *a );

    // Determine whether each operand is stored contiguously.
	x_is_contig = ( bl2_obj_vector_inc( *x ) == 1 );
	y_is_contig = ( bl2_obj_vector_inc( *y ) == 1 );
	a_is_contig = ( bl2_obj_is_row_stored( *a ) ||
	                bl2_obj_is_col_stored( *a ) );


	// Create an object to hold a copy-cast of alpha. Notice that we use
	// the type union of the target datatypes of x and y to prevent any
	// unnecessary loss of information during the computation.
	dt_alpha = bl2_datatype_union( dt_targ_x, dt_targ_y );
	bl2_obj_init_scalar_copy_of( dt_alpha,
	                             BLIS_NO_CONJUGATE,
	                             alpha,
	                             &alpha_local );

	// If all operands are contiguous, we choose a control tree for calling
	// the unblocked implementation directly without any blocking.
	if ( x_is_contig &&
	     y_is_contig &&
	     a_is_contig )
	{
		// Use different control trees depending on storage of the matrix
		// operand.
		if ( bl2_obj_is_row_stored( *a ) ) ger_cntl = ger_cntl_bs_ke_row;
		else                               ger_cntl = ger_cntl_bs_ke_col;
	}
	else
	{
		// Mark objects with unit stride as already being packed. This prevents
		// unnecessary packing from happening within the blocked algorithm.
		if ( x_is_contig ) bl2_obj_set_pack_schema( BLIS_PACKED_VECTOR, *x );
		if ( y_is_contig ) bl2_obj_set_pack_schema( BLIS_PACKED_VECTOR, *y );
		if ( a_is_contig ) bl2_obj_set_pack_schema( BLIS_PACKED_UNSPEC, *a );

		// Here, we make a similar choice as above, except that (1) we look
		// at storage tilt, and (2) we choose a tree that performs blocking.
		if ( bl2_obj_is_row_tilted( *a ) ) ger_cntl = ger_cntl_ge_row;
		else                               ger_cntl = ger_cntl_ge_col;
	}

	// Invoke the internal back-end with the copy-cast scalar and the
	// chosen control tree.
	bl2_ger_int( BLIS_NO_CONJUGATE,
	             BLIS_NO_CONJUGATE,
	             &alpha_local,
	             x,
	             y,
	             a,
	             ger_cntl );
}


//
// Define BLAS-like interfaces with homogeneous-typed operands.
//
#undef  GENTFUNC
#define GENTFUNC( ctype, ch, opname, varname ) \
\
void PASTEMAC(ch,opname)( \
                          conj_t  conjx, \
                          conj_t  conjy, \
                          dim_t   m, \
                          dim_t   n, \
                          ctype*  alpha, \
                          ctype*  x, inc_t incx, \
                          ctype*  y, inc_t incy, \
                          ctype*  a, inc_t rs_a, inc_t cs_a  \
                        ) \
{ \
	const num_t dt = PASTEMAC(ch,type); \
\
	obj_t       alphao, xo, yo, ao; \
\
	dim_t       m_x; \
	dim_t       m_y; \
	inc_t       rs_x, cs_x; \
	inc_t       rs_y, cs_y; \
\
	bl2_set_dims_with_trans( BLIS_NO_TRANSPOSE, m, n, m_x, m_y ); \
\
	rs_x = incx; cs_x = m_x * incx; \
	rs_y = incy; cs_y = m_y * incy; \
\
	bl2_obj_create_scalar_with_attached_buffer( dt, alpha, &alphao ); \
\
	bl2_obj_create_with_attached_buffer( dt, m_x, 1, x, rs_x, cs_x, &xo ); \
	bl2_obj_create_with_attached_buffer( dt, m_y, 1, y, rs_y, cs_y, &yo ); \
	bl2_obj_create_with_attached_buffer( dt, m,   n, a, rs_a, cs_a, &ao ); \
\
	bl2_obj_set_conj( conjx, xo ); \
	bl2_obj_set_conj( conjy, yo ); \
\
	PASTEMAC0(opname)( &alphao, \
	                   &xo, \
	                   &yo, \
	                   &ao ); \
}

INSERT_GENTFUNC_BASIC( ger, ger )


//
// Define BLAS-like interfaces with heterogeneous-typed operands.
//
#undef  GENTFUNC3U12
#define GENTFUNC3U12( ctype_x, ctype_y, ctype_a, ctype_xy, chx, chy, cha, chxy, opname, varname ) \
\
void PASTEMAC3(chx,chy,cha,opname)( \
                                    conj_t    conjx, \
                                    conj_t    conjy, \
                                    dim_t     m, \
                                    dim_t     n, \
                                    ctype_xy* alpha, \
                                    ctype_x*  x, inc_t incx, \
                                    ctype_y*  y, inc_t incy, \
                                    ctype_a*  a, inc_t rs_a, inc_t cs_a \
                                  ) \
{ \
	bl2_check_error_code( BLIS_NOT_YET_IMPLEMENTED ); \
}

INSERT_GENTFUNC3U12_BASIC( ger, ger )
 
#ifdef BLIS_ENABLE_MIXED_DOMAIN_SUPPORT
INSERT_GENTFUNC3U12_MIX_D( ger, ger )
#endif

#ifdef BLIS_ENABLE_MIXED_PRECISION_SUPPORT
INSERT_GENTFUNC3U12_MIX_P( ger, ger )
#endif
