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


#undef  GENTFUNCCO
#define GENTFUNCCO( ctype, ctype_r, ch, chr, varname ) \
\
void PASTEMAC(ch,varname)( \
                           ctype_r* restrict ar, \
                           ctype_r* restrict br, \
                           ctype*   restrict c, inc_t rs_c, inc_t cs_c, \
                           auxinfo_t*        data  \
                         ) \
{ \
	const dim_t       m     = PASTEMAC(chr,mr); \
	const dim_t       n     = PASTEMAC(chr,nr); \
\
	const inc_t       ps_a  = bli_auxinfo_ps_a( data ); \
	const inc_t       ps_b  = bli_auxinfo_ps_b( data ); \
\
	ctype_r* restrict a_r   = ( ctype_r* )ar; \
	ctype_r* restrict a_i   = ( ctype_r* )ar + ps_a; \
\
	ctype_r* restrict b_r   = ( ctype_r* )br; \
	ctype_r* restrict b_i   = ( ctype_r* )br + ps_b; \
\
	const inc_t       rs_a  = 1; \
	const inc_t       cs_a  = PASTEMAC(chr,packmr); \
\
	const inc_t       rs_b  = PASTEMAC(chr,packnr); \
	const inc_t       cs_b  = 1; \
\
	dim_t             iter, i, j, l; \
	dim_t             n_behind; \
\
\
	for ( iter = 0; iter < m; ++iter ) \
	{ \
		i         = iter; \
		n_behind  = i; \
\
		ctype_r* restrict alpha11_r = a_r + (i  )*rs_a + (i  )*cs_a; \
		ctype_r* restrict alpha11_i = a_i + (i  )*rs_a + (i  )*cs_a; \
		ctype_r* restrict a10t_r    = a_r + (i  )*rs_a + (0  )*cs_a; \
		ctype_r* restrict a10t_i    = a_i + (i  )*rs_a + (0  )*cs_a; \
		ctype_r* restrict b1_r      = b_r + (i  )*rs_b + (0  )*cs_b; \
		ctype_r* restrict b1_i      = b_i + (i  )*rs_b + (0  )*cs_b; \
		ctype_r* restrict B0_r      = b_r + (0  )*rs_b + (0  )*cs_b; \
		ctype_r* restrict B0_i      = b_i + (0  )*rs_b + (0  )*cs_b; \
\
		/* b1 = b1 - a10t * B0; */ \
		/* b1 = b1 / alpha11; */ \
		for ( j = 0; j < n; ++j ) \
		{ \
			ctype_r* restrict beta11_r  = b1_r + (0  )*rs_b + (j  )*cs_b; \
			ctype_r* restrict beta11_i  = b1_i + (0  )*rs_b + (j  )*cs_b; \
			ctype_r* restrict b01_r     = B0_r + (0  )*rs_b + (j  )*cs_b; \
			ctype_r* restrict b01_i     = B0_i + (0  )*rs_b + (j  )*cs_b; \
			ctype*   restrict gamma11   = c    + (i  )*rs_c + (j  )*cs_c; \
			ctype_r           beta11c_r = *beta11_r; \
			ctype_r           beta11c_i = *beta11_i; \
			ctype_r           rho11_r; \
			ctype_r           rho11_i; \
\
			/* beta11 = beta11 - a10t * b01; */ \
			PASTEMAC(chr,set0s)( rho11_r ); \
			PASTEMAC(chr,set0s)( rho11_i ); \
			for ( l = 0; l < n_behind; ++l ) \
			{ \
				ctype_r* restrict alpha10_r = a10t_r + (l  )*cs_a; \
				ctype_r* restrict alpha10_i = a10t_i + (l  )*cs_a; \
				ctype_r* restrict beta01_r  = b01_r  + (l  )*rs_b; \
				ctype_r* restrict beta01_i  = b01_i  + (l  )*rs_b; \
\
				PASTEMAC(ch,axpyris)( *alpha10_r, \
				                      *alpha10_i, \
				                      *beta01_r, \
				                      *beta01_i, \
				                      rho11_r, \
				                      rho11_i ); \
			} \
			PASTEMAC(ch,subris)( rho11_r, \
			                     rho11_i, \
			                     beta11c_r, \
			                     beta11c_i ); \
\
			/* beta11 = beta11 / alpha11; */ \
			/* NOTE: The INVERSE of alpha11 (1.0/alpha11) is stored instead
			   of alpha11, so we can multiply rather than divide. We store 
			   the inverse of alpha11 intentionally to avoid expensive
			   division instructions within the micro-kernel. */ \
			PASTEMAC(ch,scalris)( *alpha11_r, \
				                  *alpha11_i, \
			                      beta11c_r, \
				                  beta11c_i ); \
\
			/* Output final result to matrix c. */ \
			PASTEMAC(ch,sets)( beta11c_r, \
			                   beta11c_i, *gamma11 ); \
\
			/* Store the local values back to b11. */ \
			PASTEMAC(chr,copys)( beta11c_r, *beta11_r ); \
			PASTEMAC(chr,copys)( beta11c_i, *beta11_i ); \
		} \
	} \
}

INSERT_GENTFUNCCO_BASIC0( trsm4m_l_ukr_ref )

