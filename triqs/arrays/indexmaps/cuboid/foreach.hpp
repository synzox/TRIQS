/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2011 by O. Parcollet
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
#ifndef TRIQS_ARRAYS_INDEXMAP_CUBOID_FOREACH_H 
#define TRIQS_ARRAYS_INDEXMAP_CUBOID_FOREACH_H
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
//#include <boost/preprocessor/control/if.hpp>
//#include <boost/preprocessor/arithmetic/sub.hpp>
//#include <boost/type_traits/add_const.hpp>
//#include <boost/mpl/if.hpp>
#include "../../impl/mini_vector.hpp"
#include "../permutation.hpp"
#include "./map.hpp"

namespace triqs { namespace arrays { namespace indexmaps { 

// typedef std::ptrdiff_t foreach_int_type; 
 // better to be signed here : 1) on some machine/compiler, it is a lot faster !
 // When used with clef auto assign, e.g. A(i_,j_) = i -2*j, one needs signed arithmetics
 // The clef adapters would convert, but this requires a conversion at each call....
  typedef size_t foreach_int_type;

 template< class IndexMap, class Function, typename ValueType, typename Enable=void> struct foreach_impl;

 /**
  * Given : 
  *  - an object x of type T by reference (array, matrix, vector or view).
  *  - [OR] an expression expr of type Expr by const reference 
  *  - a function F (T::value_type &, T::indexmap_type::domain_type::index_value_type)
  *  it runs the loop : 
  *   for (i)
  *     for (j)
  *       ...
  *         F( T(i,j,...), tuple(i,j,k,l))
  * 
  *  Similar action can be obtained with iterators, but on some compilers & computations
  *  foreach can be faster 
  *  It is also easier to thread ??
  *
  *  NB : F is passed by value, hence copied by default. 
  *     to pass a reference, use boost::ref.
  */
 template <typename T, typename Function> 
  typename std::enable_if<std::is_base_of<Tag::indexmap_storage_pair,T>::value >::type 
  foreach( Function F, T & x) { 
   typedef typename T::value_type v;
   typedef typename boost::mpl::if_<boost::is_const<T>, typename boost::add_const<v>::type,v>::type value_type;
   typedef typename T::indexmap_type indexmap_type;
   foreach_impl<indexmap_type, Function, value_type>::invoke(x.data_start(),x.indexmap(),F);
  }

 template <typename T, typename Function> 
  typename std::enable_if<std::is_base_of<Tag::indexmap_storage_pair,T>::value >::type 
  foreach_v ( Function F, T & x) { 
   typedef typename T::value_type v;
   typedef typename boost::mpl::if_<boost::is_const<T>, typename boost::add_const<v>::type,v>::type value_type;
   typedef typename T::indexmap_type indexmap_type;
   foreach_impl<indexmap_type, Function, value_type>::invoke_v(x.data_start(),x.indexmap(),F);
  }

  template <typename T, typename Function> 
  typename std::enable_if<std::is_base_of<Tag::indexmap_storage_pair,T>::value >::type 
  foreach_av ( Function F, T & x) { 
   typedef typename T::value_type v;
   typedef typename boost::mpl::if_<boost::is_const<T>, typename boost::add_const<v>::type,v>::type value_type;
   typedef typename T::indexmap_type indexmap_type;
   foreach_impl<indexmap_type, Function, value_type>::invoke_av(x.data_start(),x.indexmap(),F);
  }

  template <typename Expr, typename Function> 
  typename std::enable_if< ! std::is_base_of<Tag::indexmap_storage_pair,Expr>::value >::type 
  foreach( Function F, Expr const & x) { for (auto & pt : x.domain()) boost::unwrap_ref(F)(x[pt],pt); }

 //--------------  IMPLEMENTATION -----------------------
 // to do : split into a foreach in ../ and a specialisation here.
#define AUX0(z,P,NNN) constexpr int p##P = mem_layout::memory_rank_to_index(cuboid::map<Rank,Opt,To>::traversal_order,NNN-P);
#define AUX0D(z,P,NNN) const int p##P = mem_layout::memory_rank_to_index(CM.memory_indices_layout_ull(),NNN-P);
#define AUX1(z,P,unused) for (t[p##P]=0; t[p##P]< l[p##P]; ++t[p##P])
#define AUX2(z,p,unused) BOOST_PP_IF(p,+,) t[p] * s[p] 
#define IMPL(z, NN, unused)                                \
 template<int Rank, ull_t Opt, ull_t To, typename Function, typename ValueType>\
 struct foreach_impl <cuboid::map<Rank,Opt,To>,Function,ValueType,typename boost::enable_if_c<(Rank==BOOST_PP_INC(NN))>::type > {\
  static void invoke ( ValueType * restrict p, cuboid::map<Rank,Opt,To> const & CM, Function F) { \
   mini_vector<foreach_int_type, Rank> t;\
   BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX0,NN)\
   const mini_vector<foreach_int_type, Rank>  l(CM.lengths());\
   const mini_vector<foreach_int_type, Rank>  s(CM.strides());\
   BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX1,nil)\
   {\
    boost::unwrap_ref(F)( p[BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX2,nil)], t );\
   } }\
  static void invoke_v ( ValueType * restrict p, cuboid::map<Rank,Opt,To> const & CM, Function F) { \
   mini_vector<foreach_int_type, Rank> t;\
   BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX0,NN)\
   const mini_vector<foreach_int_type, Rank>  l(CM.lengths());\
   const mini_vector<foreach_int_type, Rank>  s(CM.strides());\
   BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX1,nil)\
   {\
    boost::unwrap_ref(F)(  p[BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX2,nil)], BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_INC(NN),t[p,] BOOST_PP_INTERCEPT) );\
   } }\
  static void invoke_av ( ValueType * restrict p, cuboid::map<Rank,Opt,To> const & CM, Function F) { \
   mini_vector<foreach_int_type, Rank> t;\
   BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX0,NN)\
   const mini_vector<foreach_int_type, Rank>  l(CM.lengths());\
   const mini_vector<foreach_int_type, Rank>  s(CM.strides());\
   BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX1,nil)\
   {\
     p[BOOST_PP_REPEAT(BOOST_PP_INC(NN),AUX2,nil)] = boost::unwrap_ref(F)( BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_INC(NN),t[p,] BOOST_PP_INTERCEPT));\
   } }\
 };
  BOOST_PP_REPEAT(ARRAY_NRANK_MAX , IMPL, nil);
#undef IMPL
#undef AUX0
#undef AUX1
#undef AUX2
#undef PP

}}}//namespace
#endif
