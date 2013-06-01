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
#ifndef TRIQS_GF_MATSUBARA_TIME_H
#define TRIQS_GF_MATSUBARA_TIME_H
#include "./tools.hpp"
#include "./gf.hpp"
#include "./local/tail.hpp"
#include "./domains/matsubara.hpp"
#include "./meshes/linear.hpp"

namespace triqs { namespace gf {

 struct imtime {

  /// A tag to recognize the function
  struct tag {};

  /// The domain
  typedef matsubara_domain<false> domain_t;

  /// The Mesh
  typedef linear_mesh<domain_t> mesh_t;

  /// The tail
  typedef local::tail singularity_t;

  /// Symmetry
  typedef nothing symmetry_t;

  static std::string h5_name() { return "GfImTime";}
 };


 /// ---------------------------  closest mesh point on the grid ---------------------------------

 template<>
  struct get_closest_point <imtime> {
   // index_t is size_t
   template<typename G, typename T>
    static size_t invoke(G const * g, closest_pt_wrap<T> const & p) {
     double x = (g->mesh().kind()==half_bins ? double(p.value) :  double(p.value)+ 0.5*g->mesh().delta());
     size_t n = std::floor(x/g->mesh().delta());
     return n;
    }
  };


 /// ---------------------------  evaluator ---------------------------------

 template<>
  struct evaluator<imtime> {
   private:
    mutable arrays::matrix<double> _tmp;
   public :
    static constexpr int arity = 1;
    evaluator<imtime>() = default;
    evaluator<imtime>(size_t n1, size_t n2) : _tmp(n1,n2) {}
    // WHAT happen in resize ??

    /*
    template<typename G, typename T>
     arrays::const_matrix_view_proxy<typename G::data_t,0> operator()(G const * g, closest_pt_wrap<T> const & p) const {
      double x = (g->mesh().kind()==half_bins ? double(p.value) :  double(p.value)+ 0.5*g->mesh().delta());
      size_t n = std::floor(x/g->mesh().delta());
      return arrays::const_matrix_view_proxy<typename G::data_t,0>(g->data(),n);
     }
*/
    // NOT TESTED
    // TEST THE SPPED when q_view are incorporated...
    // true evaluator with interpolation ...
    template<typename G>
     arrays::matrix<double> const & operator()(G const * g, double tau) const {
      // interpolate between n and n+1, with weight
      double beta = g->mesh().domain().beta;
      int p = std::floor(tau/beta);
      tau -= p*beta;
      double a = tau/g->mesh().delta();
      long n = std::floor(a);
      double w = a-n;
      assert(n < g->mesh().size()-1);
      if ((g->mesh().domain().statistic == Fermion) && (p%2==1))
       _tmp = - w*g->data()(n, arrays::range(), arrays::range()) - (1-w)*g->data()(n+1, arrays::range(), arrays::range());
      else
       _tmp =   w*g->data()(n, arrays::range(), arrays::range()) + (1-w)*g->data()(n+1, arrays::range(), arrays::range());
      //else { // Speed test to redo when incoparated qview in main branch
      // _tmp(0,0) =   w*g->data()(n, 0,0) + (1-w)*g->data()(n+1, 0,0);
      // _tmp(0,1) =   w*g->data()(n, 0,1) + (1-w)*g->data()(n+1, 0,1);
      // _tmp(1,0) =   w*g->data()(n, 1,0) + (1-w)*g->data()(n+1, 1,0);
      // _tmp(1,1) =   w*g->data()(n, 1,1) + (1-w)*g->data()(n+1, 1,1);
      // }
      return _tmp;
     }

    template<typename G>
     typename G::singularity_t const & operator()(G const * g,freq_infty const &) const {return g->singularity();}
  };

 /// ---------------------------  data access  ---------------------------------

 template<> struct data_proxy<imtime> : data_proxy_array<double,3> {};

 // -------------------  ImmutableGfMatsubaraTime identification trait ------------------

 template<typename G> struct ImmutableGfMatsubaraTime : std::is_base_of<typename imtime::tag,G> {};

 // -------------------------------   Factories  --------------------------------------------------

 template<> struct gf_factories< imtime> : imtime {
  typedef gf<imtime> gf_t;

  static mesh_t make_mesh(double beta, statistic_enum S, size_t n_time_slices, mesh_kind mk=half_bins) {
   return mesh_t(domain_t(beta,S), 0, beta, n_time_slices, mk);
  }

  template<typename MeshType>
   static gf_t make_gf(MeshType && m, tqa::mini_vector<size_t,2> shape, local::tail_view const & t) {
    gf_t::data_non_view_t A(shape.front_append(m.size())); A() =0;
    //return gf_t ( m, std::move(A), t, nothing() ) ;
    return gf_t (std::forward<MeshType>(m), std::move(A), t, nothing(), evaluator<imtime>(shape[0],shape[1]) ) ;
   }
  /*static gf_t make_gf(double beta, statistic_enum S, tqa::mini_vector<size_t,2> shape) {
    return make_gf(make_mesh(beta,S,1025,half_bins), shape, local::tail(shape));
    }
    static gf_t make_gf(double beta, statistic_enum S, tqa::mini_vector<size_t,2> shape, size_t Nmax) {
    return make_gf(make_mesh(beta,S,Nmax,half_bins), shape, local::tail(shape));
    }
    */
  static gf_t make_gf(double beta, statistic_enum S,  tqa::mini_vector<size_t,2> shape, size_t Nmax=1025, mesh_kind mk= half_bins) {
   return make_gf(make_mesh(beta,S,Nmax,mk), shape, local::tail(shape));
  }
  static gf_t make_gf(double beta, statistic_enum S, tqa::mini_vector<size_t,2> shape, size_t Nmax, mesh_kind mk, local::tail_view const & t) {
   return make_gf(make_mesh(beta,S,Nmax,mk), shape, t);
  }
 };
}}
#endif

