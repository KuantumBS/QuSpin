from __future__ import print_function, division

import sys,os
qspin_path = os.path.join(os.getcwd(),"../")
sys.path.insert(0,qspin_path)

from quspin.operators import hamiltonian
from scipy.sparse import random,dia_matrix
import numpy as np
from itertools import product

dtypes = [np.float32,np.float64,np.complex64,np.complex128]
formats = ["dia","csr","csc"]

def eps(N,dtype1,dtype2):
	return N*max(np.finfo(dtype1).eps,np.finfo(dtype2).eps)

def func(t):
	return t

def func_cmplx(t):
	return 1j*t


N = 1000
for fmt in formats:
	for dtype1,dtype2 in product(dtypes,dtypes):
		for i in range(10):
			print("testing {} {} {} {}".format(fmt,dtype1,dtype2,i+1))
			if fmt in ["csr","csc"]:
			
				A = (random(N,N,density=np.log(N)/N) + 1j*random(N,N,density=np.log(N)/N))
				A = (A + A.H)/2.0
				A = A.astype(dtype1).asformat(fmt)
			else:
				ndiags = N//10
				diags = np.random.uniform(0,1,size=(ndiags,N))+1j*np.random.uniform(0,1,size=(ndiags,N))
				diags = diags.astype(dtype1)
				offsets = np.random.choice(np.arange(-N//2+1,N//2,1),size=ndiags,replace=False)
				A = dia_matrix((diags,offsets),shape=(N,N),dtype=dtype1)

			v = np.random.uniform(-1,1,size=N) + 1j * np.random.uniform(-1,1,size=N)
			v = v.astype(dtype2)

			if dtype1 in [np.complex128,np.complex64]:
				H = hamiltonian([],[[A,func_cmplx,()]],static_fmt=fmt,dtype=dtype1)
			else:
				H = hamiltonian([],[[A,func,()]],static_fmt=fmt,dtype=dtype1)


			out = np.zeros_like(v,dtype=np.result_type(dtype1,dtype2))

			t = np.random.normal(0,1)

			if dtype1 in [np.complex128,np.complex64]:
				res2 = 1j*t*(A.dot(v))
			else:
				res2 = t*(A.dot(v))

			res1 = H.dot(v,time=t)
			H.dot(v,time=t,out=out,overwrite_out=True)


			result_dtype = np.result_type(dtype1,dtype2)
			atol = eps(N,dtype1,dtype2)
			try:
				np.testing.assert_allclose(res1,res2,atol=atol)
			except AssertionError,e:
				print(res1-res2, atol)
				raise AssertionError(e)

			try:
				np.testing.assert_allclose(res1,out,atol=atol)
			except AssertionError,e:
				print(res1-out, atol)
				raise AssertionError(e)

			try:
				np.testing.assert_allclose(out,res2,atol=atol)
			except AssertionError,e:
				print(out-res2, atol)
				raise AssertionError(e)

			


print("oputils tests passed!")





