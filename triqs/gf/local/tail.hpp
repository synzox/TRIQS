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
#ifndef TRIQS_GF_LOCAL_TAIL_H
#define TRIQS_GF_LOCAL_TAIL_H
#include <triqs/clef/core.hpp>
#include <triqs/arrays/array.hpp>
#include <triqs/arrays/matrix.hpp>
#include <triqs/utility/proto/tools.hpp>
#include <triqs/arrays/h5/simple_read_write.hpp>
#include <triqs/arrays/proto/matrix_algebra.hpp>
#include <triqs/arrays/proto/array_algebra.hpp>
#include "triqs/utility/complex_ops.hpp"
#include <triqs/utility/view_tools.hpp>

#include "../meshes.hpp"
#include "../domains.hpp"

namespace triqs { namespace gf { namespace local {

 namespace mpl= boost::mpl; 
 class tail;                            // the value class
 class tail_view;                       // the view class
 template<typename E>  class tail_expr; // the proto expression of tail

 template<typename G> struct LocalTail  : mpl::false_{};  // a boolean trait to identify the objects modelling the concept LocalTail
 template<> struct LocalTail<tail >     : mpl::true_{};
 template<> struct LocalTail<tail_view >: mpl::true_{};
 template<typename M> struct LocalTail<tail_expr<M> >: mpl::true_{};

 typedef std::complex<double> dcomplex;

 // ---------------------- implementation --------------------------------

 namespace tqa= triqs::arrays; namespace tql= triqs::clef;  namespace proto=boost::proto; 
 namespace bf = boost::fusion; namespace tup = triqs::utility::proto; 
 
 /// A common implementation class. Idiom : ValueView
 template<bool IsView> class tail_impl  {  
  public : 
   typedef void has_view_type_tag; // Idiom : ValueView 
   typedef tail_view view_type;
   typedef tail      non_view_type;
   
   typedef arrays::Option::Fortran storage_order;
   typedef arrays::array      <dcomplex,3,storage_order>                         data_non_view_type;
   typedef arrays::array_view <dcomplex,3,storage_order>                         data_view_type;
   typedef typename mpl::if_c<IsView, data_view_type, data_non_view_type>::type  data_type;

   typedef arrays::matrix_view<dcomplex,       storage_order>  mv_type;
   typedef arrays::matrix_view<const dcomplex, storage_order>  const_mv_type;

   data_view_type data_view()             { return data;} 
   const data_view_type data_view() const { return data;}

   int order_min() const {return omin;}
   int order_max() const {return omin + size() -1;}
   size_t size()   const {return data.shape()[2];} 

   typedef tqa::mini_vector<size_t,2> shape_type;
   shape_type shape() const { return shape_type(data.shape()[0], data.shape()[1]);} 

  protected:
   int omin;
   data_type data;

   tail_impl(): omin(0),data() {} // all arrays of zero size (empty)
   tail_impl (size_t N1, size_t N2, int order_min, size_t size_) : omin(order_min), data(tqa::make_shape(N1,N2,size_)){data()=0;}
   tail_impl( data_type const &d, int order_min ): omin(order_min), data(d){}
   tail_impl(tail_impl          const & x): omin(x.omin), data(x.data){}
   tail_impl(tail_impl<!IsView> const & x): omin(x.omin), data(x.data){}

   friend class tail_impl<!IsView>;
  public:
   
   void operator = (tail_impl          const & x) { omin = x.omin; data = x.data;}
   void operator = (tail_impl<!IsView> const & x) { omin = x.omin; data = x.data;}
   
   template<typename RHS> ENABLE_IF(LocalTail<RHS>) operator = (RHS const & rhs) { // the general version for an expression
    // If it is a view, we can not resize, but we can truncate the rhs and we do it
    // If it is not a view, we resize
     if (IsView) { if (size()>rhs.size()) TRIQS_RUNTIME_ERROR <<" tail : operator = : rhs is too short !";}
     else {
      shape_type rhs_shape =  rhs.shape(); // in case it would  be long to compute
      tqa::resize_or_check_if_view ( data, tqa::make_shape(rhs_shape[0],rhs_shape[1], rhs.size() )); 
     }
     omin = rhs.order_min(); 
     int i=omin; for (size_t u=0; u<size(); ++u,++i) data(tqa::range(),tqa::range(),u) = rhs(i); 
    }

   mv_type operator() (int n)       { 
    if (n>this->order_max()) TRIQS_RUNTIME_ERROR<<" n > Max Order. n= "<<n <<", Max Order = "<<order_max() ; 
    if (n<this->order_min()) TRIQS_RUNTIME_ERROR<<" n < Min Order. n= "<<n <<", Min Order = "<<order_min() ;
    return this->data(tqa::range(), tqa::range(), n- this->order_min());
   }

   const_mv_type operator() (int n) const {  
    if (n>this->order_max()) TRIQS_RUNTIME_ERROR<<" n > Max Order. n= "<<n <<", Max Order = "<<order_max() ;
    if (n<this->order_min())  { mv_type::non_view_type r(this->shape()); r()=0; return r;} 
    return this->data(tqa::range(), tqa::range(), n- this->order_min());
   }

   operator domains::freq_infty() const { return domains::freq_infty();}

   /// Save in txt file : doc the format  ? ---> prefer serialization or hdf5 !
   void save(std::string file,  bool accumulate=false) const {}

   /// Load from txt file : doc the format ?
   void load(std::string file){}

   /// 
   friend void h5_write (tqa::h5::group_or_file fg, std::string subgroup_name, tail_impl const & t) {
    BOOST_AUTO( gr , fg.create_group(subgroup_name) );
    // Add the attribute
    h5_write(gr,"omin",t.omin);
    h5_write(gr,"data",t.data);
   }

   friend void h5_read  (tqa::h5::group_or_file fg, std::string subgroup_name, tail_impl & t){
    BOOST_AUTO( gr,  fg.open_group(subgroup_name) );
    // Check the attribute or throw 
    h5_read(gr,"omin",t.omin);
    h5_read(gr,"data",t.data);
   }

   //  BOOST Serialization
   friend class boost::serialization::access;
   template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
     ar & boost::serialization::make_nvp("omin",omin);
     ar & boost::serialization::make_nvp("data",data);
    }

   friend std::ostream & operator << (std::ostream & out, tail_impl const & x) { 
    out <<" tail/tail_view : Ordermin/max = "<< x.order_min() << "  "<< x.order_max();
    for (int u =x.order_min();u<= x.order_max(); ++u) out <<"\n ...  Order "<<u << " = " << x(u);
    return out; 
   } 

 };

 // -----------------------------
 ///The View class of GF
 class tail_view : public tail_impl <true> { 
  typedef tail_impl <true>  B;
  public :
  template<bool V> tail_view(tail_impl<V> const & t): B(t){}
  tail_view(B::data_type const &d, int order_min): B(d,order_min){}

  using B::operator=; // import operator = from impl. class or the default = is synthetized 
  using B::operator(); // import all previously defined operator() for overloading
  TRIQS_CLEF_ADD_LAZY_CALL_WITH_VIEW(1,tail_view); // add lazy call, using the view in the expression tree.
  friend std::ostream & triqs_nvl_formal_print(std::ostream & out, tail_view const & x) { return out<<"tail_view";}
 };

 // -----------------------------
 ///The regular class 
 class tail : public tail_impl <false> { 
  typedef tail_impl <false>  B;
  public : 
  tail():B() {} 
  tail(size_t N1, size_t N2,  int order_min, size_t size_ ):B(N1,N2,order_min,size_) {} 
  tail(tail const & g): B(g){}
  tail(tail_view const & g): B(g){} 
  template<typename GfType> tail(GfType const & x): B() { *this = x;} // to maintain value semantics

  using B::operator=;
  using B::operator();
  TRIQS_CLEF_ADD_LAZY_CALL_WITH_VIEW(1,tail_view); // add lazy call, using the view in the expression tree.

  /// The simplest tail corresponding to  : omega
  static tail_view omega(size_t N1, size_t N2, size_t size_) { tail t(N1,N2,-1, size_); t(t.order_min())=1; return t; }

  /// The simplest tail corresponding to  : omega, constructed from a shape for convenience
  static tail_view omega(tail::shape_type const & sh, size_t size_) { return omega(sh[0],sh[1],size_);}
 };

 /// Slice in orbital space
 template<bool V> tail_view slice(tail_impl<V> const & t, tqa::range R1, tqa::range R2) { 
  return tail_view(t.data_view()(R1,R2,tqa::range()),t.order_min());
 } 

 // -------------------------------   Expression template for tail and view -----------------------

 typedef tqa::matrix_view<std::complex<double>, tail::storage_order > mv_dcomplex_type;

 // -----------  tail special multiplication --------------------

 template<typename L, typename R> struct tail_mul_lazy { // should have the same concept as tail_expr
  typename const_view_type_if_exists_else_type<L>::type l; 
  typename const_view_type_if_exists_else_type<R>::type r;
  tail_mul_lazy(L const & l_, R const & r_):l(l_),r(r_){ if (l.shape()[1] != r.shape()[0]) TRIQS_RUNTIME_ERROR<< "tail multiplication : shape mismatch";}

  int order_min() const { return l.order_min() + r.order_min();}
  size_t size()   const { return std::min(l.size(),r.size());}
  int order_max() const { return order_min() + size() -1;}

  typedef tqa::mini_vector<size_t,2> shape_type;
  shape_type shape() const {return shape_type(l.shape()[0], r.shape()[1]);} 

  mv_dcomplex_type operator ()(int n) const {
   // sum_{p}^n a_p b_{n-p}. p <= a.n_max, p >= a.n_min and n-p <=b.n_max and n-p >= b.n_min
   // hence p <= min ( a.n_max, n-b.n_min ) and p >= max ( a.n_min, n- b.n_max)  
   const int pmin = std::max(l.order_min(), n - r.order_max() );
   const int pmax = std::min(l.order_max(), n - r.order_min() );
   mv_dcomplex_type::non_view_type res(shape()[0],shape()[1]); res()=0;
   for (int p = pmin; p <= pmax; ++p)  { res += l(p) * r(n-p);} 
   return res;
  }
 };

 // -----------  

 template<typename L, typename R> struct LocalTail<tail_mul_lazy<L,R> >: mpl::true_{};

 template<typename T1, typename T2> 
  typename boost::enable_if< mpl::and_<LocalTail<T1>, LocalTail<T2> >, tail_mul_lazy<T1,T2> >::type
  operator* (T1 const & a, T2 const & b) { return tail_mul_lazy<T1,T2>(a,b); }

 // -----------  tail special inversion --------------------

 template<typename T> struct tail_inv_lazy { // should have the same concept as tail_expr
  typedef typename const_view_type_if_exists_else_type<T>::type t_type; 
  t_type t; 
  tail_inv_lazy(T const & t_):t(t_){}

  int order_min() const { return - t.order_min(); }
  size_t size()   const { return t.size();}
  int order_max() const { return order_min() + size() -1;}

  typedef tqa::mini_vector<size_t,2> shape_type;
  shape_type shape() const {return t.shape();}

  struct internal_data { // implementing the pattern LazyPreCompute
   t_type const & t;
   tail t_inv;
   internal_data( tail_inv_lazy const & Parent): t(Parent.t),
   t_inv(t.shape()[0],t.shape()[1] , Parent.order_min(), Parent.size()) {
    // b_n = - a_0^{-1} * sum_{p=0}^{n-1} b_p a_{n-p} for n>0
    // b_0 = a_0^{-1}
    // b_min <= p <=b_max ;  a_min <= n-p <= a_max ---> n-a_max  <= p <= n-a_min 
    const int omin = t_inv.order_min(); const int omax = t_inv.order_max();
    t_inv(omin) = inverse(t(t.order_min()));
    for (int n=1; n< t_inv.size();n++) {
     // 0<= n-p < t.size() ---> p <= n, ok  and p > n- t.size() 
     const int pmin = std::max(0, n - int(t.size()) +1 );
     for (int p=pmin; p< n ; p++) { t_inv(omin + n) -= t_inv(omin + p)*t(t.order_min() + n-p); }
     t_inv(omin + n) *= t_inv(omin);
    }
   }
  };
  friend struct internal_data;
  mutable boost::shared_ptr<internal_data> _id;
  internal_data const & id() const { if (!_id) _id= boost::make_shared<internal_data>(*this); return *_id;}

  mv_dcomplex_type operator ()(int n) const { return id().t_inv(n); }

  friend std::ostream & operator << (std::ostream & out, tail_inv_lazy const & x) { return out <<" tail_inv_lazy";}
 };

 // -----------  

 template<typename T> struct LocalTail<tail_inv_lazy<T> >: mpl::true_{};

 template<typename T> typename boost::enable_if< LocalTail<T>, tail_inv_lazy <T> >::type 
  inverse (T const & t) { return tail_inv_lazy<T>(t);}

 template<typename A, typename T> // anything / tail ---> anything * inverse(tail)
  typename boost::lazy_enable_if< LocalTail<T>, tup::type_of_mult<A, tail_inv_lazy <T> > >::type 
  operator/ (A const & a, T const & t) { return a * tail_inv_lazy<T>(t);}

 // -----------  expression  --------------------

 // a trait to find the scalar of the algebra i.e. the true scalar and the matrix ...
 template <typename T> struct is_scalar_or_element : mpl::or_< tqa::ImmutableMatrix<T>, tup::is_in_ZRC<T> > {};
 struct tail_scalar_grammar : proto::and_< proto::terminal<proto::_>, proto::if_<is_scalar_or_element<proto::_value>()> > {}; 

 struct tail_leaf_grammar: proto::and_< proto::terminal<proto::_>, proto::if_<LocalTail<proto::_value>()> > {}; 

 struct tail_grammar : 
  proto::or_<
  tail_scalar_grammar , tail_leaf_grammar
  , proto::plus      <tail_grammar ,tail_grammar>
  , proto::minus     <tail_grammar ,tail_grammar>
  , proto::multiplies<tail_grammar ,tail_scalar_grammar>
  , proto::multiplies<tail_scalar_grammar,tail_grammar>
  , proto::divides   <tail_grammar ,tail_scalar_grammar>
  , proto::negate    <tail_grammar >
  > {};

 struct tail_eval_scalar_t { // a transform that evaluates a scalar as a trivial expansion (only order 0)
  BOOST_PROTO_CALLABLE();
  template<typename Sig> struct result;
  template<typename This, typename S, typename Arg> struct result<This(S,Arg)> { typedef dcomplex type;};
  template<typename S, typename Arg> dcomplex operator ()(S const & s, Arg const & arg) const {int n = arg; return (n==0?s:S());}
 };

 struct tail_eval_leaf_t { // a transform that evaluates a leaf of the expression tree
  BOOST_PROTO_CALLABLE();
  template<typename Sig> struct result;
  template<typename This, typename T, typename Arg> struct result<This(T,Arg)> { typedef mv_dcomplex_type type;};
  template<typename T, typename Arg> mv_dcomplex_type operator ()(T const & t, Arg const & arg) const { return t(arg);}
 };

 struct tail_eval_t : // evaluation of an expression. same as for gf.  
  proto::or_<
  proto::when<tail_scalar_grammar, tail_eval_scalar_t(proto::_value, proto::_state)>
  ,proto::when<tail_leaf_grammar, tail_eval_leaf_t(proto::_value, proto::_state) >
  ,proto::when< proto::multiplies<tail_scalar_grammar,tail_eval_t>, tup::multiplies_t (proto::_value(proto::_left),tail_eval_t(proto::_right)) > 
  ,proto::when< proto::multiplies<tail_eval_t,tail_scalar_grammar>, tup::multiplies_t (proto::_value(proto::_right),tail_eval_t(proto::_left)) > 
  ,proto::when<proto::nary_expr<proto::_, proto::vararg<proto::_> >,  proto::_default<tail_eval_t > >
  > {};

 //------ computation of the shape, orders of the expression. similar as for gf, except the run time check is more relax ----

 struct no_result{}; // (for scalar)
 struct sh_or { tqa::mini_vector<size_t,2> shape; int order_min, order_max;}; 

 struct tail_combine_shape_t { // a transform that combine the mesh of a node with the State
  BOOST_PROTO_CALLABLE();
  template<typename Sig> struct result;
  template<typename This, typename X, typename S> struct result<This(X,S)> {
   typedef sh_or type;
   typedef typename tup::remove_const_and_ref<S>::type S2;
   static_assert((mpl::or_<boost::is_same<S2,no_result>, boost::is_same<type,S2> >::value), "FATAL : two meshs of different type mixed in an expression");   
  };
  template<typename X> sh_or operator ()(X const & x, no_result ) const { 
   sh_or r; r.shape = x.shape(); r.order_min =x.order_min(); r.order_max =x.order_max(); return r;
  }
  template<typename X, typename M> sh_or operator ()(X const & x, M const & m ) const { 
   if (x.shape() != m.shape) TRIQS_RUNTIME_ERROR<<" Shape mismatch";
   sh_or r; r.shape = x.shape(); r.order_min =std::min (x.order_min(), m.order_min); r.order_max =std::min (x.order_max(), m.order_max); return r;
  }
 };

 struct tail_shape_t : // the transform that computes the mesh recursively using a fold and a State initialized to no_result
  proto::or_<
  proto::when < tail_scalar_grammar, proto::_state >
  ,proto::when< tail_leaf_grammar, tail_combine_shape_t (proto::_value, proto::_state) >
  ,proto::when<proto::nary_expr<proto::_, proto::vararg<proto::_> >,  proto::fold<proto::_, proto::_state, tail_shape_t >() >
  > {};

 // grammar and domain and proto expression
 typedef tup::domain_with_copy<tail_grammar,tail_expr> tail_expr_domain;

 template<typename Expr> struct tail_expr : proto::extends<Expr, tail_expr<Expr>, tail_expr_domain> {
  tail_expr( Expr const & expr = Expr() ) : proto::extends<Expr, tail_expr<Expr>, tail_expr_domain> (expr) { init = false;}

  int order_min() const { return so().order_min; }
  int order_max() const { return so().order_max; }
  size_t size()   const {return std::max (order_max() - order_min() +1, 0); } 

  typedef tqa::mini_vector<size_t,2> shape_type;
  shape_type shape() const {return so().shape;}

  typename boost::result_of<tail_eval_t(Expr,int)>::type operator()(int n) const {return tail_eval_t()(*this,n);}
  friend std::ostream &operator <<(std::ostream &sout, tail_expr<Expr> const &expr) {return tup::print_algebra(sout,expr);} 

  private : // compute the sahpe and ordermin/max only once... 
  mutable sh_or so_; mutable bool init;
  sh_or const & so() const { if (!init) { so_ = tail_shape_t()(*this,no_result()); init = true; } return so_;} 
 };

 BOOST_PROTO_DEFINE_OPERATORS(LocalTail, tail_expr_domain);
}}}
#endif