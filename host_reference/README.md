# QUDA 1.0.0

## host_reference

This directory contains host side reference code that cross checks the GPU kerels in QUDA
with host side calculated routines. Currenly supported reference routines for matrix 
operators are:

	 1. Wilson dslash
	 2. Clover dslash
	 3. Staggered dslash
	 4. Domain Wall dslash
	 5. Covariant derivative
	
For gauge related routines, we have:

    	 1. Long/Fat link construction
	 2. Wilson gauge action force
	 3. HISQ gauge action force

And we also offer routines for dense arithmetic:

       	 1. BLAS
	 2. Spinor contration

The former will compute a wide variety of BLAS calls and, and the latter will contract
two spinors, returning an array populated with a 4x4 array of open spin index, colour 
contracted data at each lattice point.