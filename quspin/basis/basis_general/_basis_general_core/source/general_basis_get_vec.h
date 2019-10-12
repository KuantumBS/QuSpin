#ifndef _GENERAL_BASIS_GET_VEC_H
#define _GENERAL_BASIS_GET_VEC_H

#include "general_basis_core.h"
#include "numpy/ndarraytypes.h"
#include "misc.h"
#include "openmp.h"


namespace basis_general {

template<class T,class P>
bool inline update_out_dense(std::complex<double> c, P phase, npy_intp n_vec,const std::complex<T> *in, std::complex<T> *out){
	for(npy_intp i=0;i<n_vec;i++){
		out[i] += T(phase) * std::complex<T>(c) * in[i];
	}
	return true;
}

template<class T,class P>
bool inline update_out_dense(std::complex<double> c, P phase, npy_intp n_vec,const T *in, T *out){
	if(std::abs(c.imag())>1.1e-15){
		return false;
	}
	else{
		T re = c.real();
		for(npy_intp i=0;i<n_vec;i++){
			out[i] += T(phase) * re * in[i];
		}
		return true;
	}
}

template<class T>
bool inline update_out_dense(std::complex<double> c, std::complex<double> phase, npy_intp n_vec,const std::complex<T> *in, std::complex<T> *out){
	for(npy_intp i=0;i<n_vec;i++){
		out[i] += std::complex<T>(phase*c) * in[i];
	}
	return true;
}

template<class T>
bool inline update_out_dense(std::complex<double> c, std::complex<double> phase, npy_intp n_vec,const T *in, T *out){
	c *= phase;
	if(std::abs(c.imag())>1.1e-15){
		return false;
	}
	else{
		T re = c.real();
		for(npy_intp i=0;i<n_vec;i++){
			out[i] += re * in[i];
		}
		return true;
	}
}


template<class I,class T,class P=signed char>
bool get_vec_rep(general_basis_core<I,P> *B,
									 I s,
								   P &phase,
							 const int nt,
							 const npy_intp n_vec,
							 const npy_intp Ns_full,
							 const T in[],
							 std::complex<double> c,
							 	   T out[],
							 const int depth)
{
	bool err = true;
	if(nt<=0){
		const npy_intp full = (Ns_full - s - 1)*n_vec;
		err = update_out_dense(c,phase,n_vec,in,&out[full]);		
		return err;
	}
	int per = B->pers[depth];
	double q = (2.0*M_PI*B->qs[depth])/per;
	std::complex<double> cc = std::exp(std::complex<double>(0,-q));

	if(depth < nt-1){
		for(int j=0;j<per && err;j++){
			err = get_vec_rep(B,s,phase,nt,n_vec,Ns_full,in,c,out,depth+1);
			c *= cc;
			s = B->map_state(s,depth,phase);
		}
		return err;
	}
	else{
		for(int j=0;j<per && err;j++){
			const npy_intp full = (Ns_full - s - 1)*n_vec;
			err = update_out_dense(c,phase,n_vec,in,&out[full]);
			c *= cc;
			s = B->map_state(s,depth,phase);
		}
		return err;
	}
}



template<class I,class T,class P=signed char>
bool get_vec_rep_pcon(general_basis_core<I,P> *B,
									 I s,
								   P &phase,
							 const int nt,
							 const npy_intp n_vec,
							 const I basis_pcon[],
							 const npy_intp Ns_full,
							 const T in[],
							 std::complex<double> c,
							 	   T out[],
							 const int depth)
{
	bool err = true;
	if(nt<=0){
		const npy_intp full = binary_search(Ns_full,basis_pcon,s)*n_vec;
		err = update_out_dense(c,phase,n_vec,in,&out[full]);		
		return err;
	}
	int per = B->pers[depth];
	double q = (2.0*M_PI*B->qs[depth])/per;
	std::complex<double> cc = std::exp(std::complex<double>(0,-q));

	if(depth < nt-1){
		for(int j=0;j<per && err;j++){
			err = get_vec_rep_pcon(B,s,phase,nt,n_vec,basis_pcon,Ns_full,in,c,out,depth+1);
			c *= cc;
			s = B->map_state(s,depth,phase);
		}
		return err;
	}
	else{
		for(int j=0;j<per && err;j++){
			const npy_intp full = binary_search(Ns_full,basis_pcon,s)*n_vec;
			err = update_out_dense(c,phase,n_vec,in,&out[full]);
			c *= cc;
			s = B->map_state(s,depth,phase);
		}
		return err;
	}
}


template<class I,class J,class T,class P=signed char>
bool get_vec_general_pcon_dense(general_basis_core<I,P> *B,
										 const I basis[],
										 const J n[],
										 const npy_intp n_vec,
										 const npy_intp Ns,
										 const npy_intp Ns_full,
										 const I basis_pcon[],
										 const T in[],
										 	   T out[])
{
	bool err = true;
	const int nt = B->get_nt();
	const npy_intp chunk = std::max(Ns/(100*omp_get_max_threads()),(npy_intp)1);

	double norm = 1.0;

	for(int i=0;i<nt;i++){
		norm *= B->pers[i];
	}

	#pragma omp parallel for schedule(dynamic,chunk) firstprivate(norm)
	for(npy_intp k=0;k<Ns;k++){
		if(!err){continue;}

			std::complex<double> c = 1.0/std::sqrt(n[k]*norm);
			P phase = 1;
			bool local_err = get_vec_rep_pcon(B,basis[k],phase,nt,n_vec,basis_pcon,Ns_full,&in[k*n_vec],c,out,0);
			if(!local_err){
				#pragma omp critical
				err = local_err;
			}
	}

	return err;
}

template<class I,class J,class T,class P=signed char>
bool get_vec_general_dense(general_basis_core<I,P> *B,
										 const I basis[],
										 const J n[],
										 const npy_intp n_vec,
										 const npy_intp Ns,
										 const npy_intp Ns_full,
										 const T in[],
										 	   T out[])
{
	bool err = true;
	const int nt = B->get_nt();
	const npy_intp chunk = std::max(Ns/(100*omp_get_max_threads()),(npy_intp)1);

	double norm = 1.0;

	for(int i=0;i<nt;i++){
		norm *= B->pers[i];
	}


	#pragma omp parallel for schedule(dynamic,chunk) firstprivate(norm)
	for(npy_intp k=0;k<Ns;k++){
		if(!err){continue;}

		std::complex<double> c = 1.0/std::sqrt(n[k]*norm);
		P phase = 1;
		bool local_err = get_vec_rep(B,basis[k],phase,nt,n_vec,Ns_full,&in[k*n_vec],c,out,0);
		if(!local_err){
			#pragma omp critical
			err = local_err;
		}
	}

	return err;
}

}



#endif
