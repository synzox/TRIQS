/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2012 by M. Ferrero, O. Parcollet
 *
 * TRIQS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * TRIQS. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifndef TRIQS_GF_LOCAL_DOMAIN_H
#define TRIQS_GF_LOCAL_DOMAIN_H

namespace triqs { namespace gf {

 enum Statistic {Boson,Fermion};

 namespace domains {
  struct matsubara_freq{};
  struct matsubara_time{};
  struct matsubara_legendre{};
  struct real_freq {};
  struct real_time {};
  struct infty{};
 }

 namespace meshes { 

  class tail{
   size_t order_;

   public:
   typedef std::complex<double> gf_result_type;
   typedef size_t index_type;
   
   tail(size_t Order=5) : order_(Order) {}
   
   // size_t order() const { return order_;}
   
   static const bool has_tail = false;
   static const bool mesh_tail = false;
   
   size_t len() const{ return order_;}
  };

  //--------------------------------------------------------

  class matsubara_freq : domains::matsubara_freq {
   size_t n_max_; 

   public:
   typedef std::complex<double> gf_result_type;
   typedef size_t index_type;

   matsubara_freq (Statistic s=Fermion, size_t N_max=1025, size_t tail_expansion_order=5 ): 
    n_max_( N_max), statistic(s), mesh_tail(tail_expansion_order) {}

   //size_t n_max() const { return n_max_;}
   Statistic statistic;

   static const bool has_tail = true;
   tail mesh_tail;

   size_t len() const{ return n_max_;}
  };

  //--------------------------------------------------------

  class matsubara_time : domains::matsubara_freq {
   size_t n_time_slices_;
 
   public: 
   typedef double gf_result_type;
   typedef size_t index_type;
   
   matsubara_time (size_t N_time_slices, Statistic s): n_time_slices_( N_time_slices), statistic(s) {}
   
   //size_t n_time_slices() const { return n_time_slices_;}
   
   Statistic statistic;
   
   static const bool has_tail = true;
   tail mesh_tail;
   
   size_t len() const{ return n_time_slices_;}
  };

  
 }

}}

#endif



