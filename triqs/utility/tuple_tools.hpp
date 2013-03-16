/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2013 by O. Parcollet
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
#ifndef TRIQS_UTILITY_TUPLE_TOOLS_H
#define TRIQS_UTILITY_TUPLE_TOOLS_H
#include<triqs/utility/macros.hpp>
#include <tuple>
namespace triqs { namespace tuple {

 /**
  * apply(f, t)
  * f : a callable object
  * t a tuple
  * Returns : [f(i) for i in t]
  */

 template<int pos,  typename T> struct apply_impl {
  template<typename F, typename ... Args>
   auto operator()(F && f, T const & t, Args && ... args)
   DECL_AND_RETURN( apply_impl<pos-1,T>()(std::forward<F>(f),t, std::get<pos>(t), std::forward<Args>(args)...));
 };

 template<typename T> struct apply_impl<-1,T> {
  template<typename F, typename ... Args>
   auto operator()(F && f, T const & t, Args && ... args) DECL_AND_RETURN( std::forward<F>(f)(args...));
 };

 template<typename F, typename T>
  auto apply (F && f, T const & t) DECL_AND_RETURN( apply_impl<std::tuple_size<T>::value-1,T>()(std::forward<F>(f),t));


 /**
  * apply_on_zip(f, t1,t2)
  * f : a callable object
  * t1, t2 two tuples of the same size
  * Returns : [f(i,j) for i,j in zip(t1,t2)]
  */
 template<int pos, typename T1, typename T2> struct apply_on_zip_impl {
  template<typename F, typename ... Args>
   auto operator()(F && f, T1 const & t1, T2 const & t2, Args && ... args)
   DECL_AND_RETURN( apply_on_zip_impl<pos-1,T1,T2>()(std::forward<F>(f),t1, t2, f(std::get<pos>(t1),std::get<pos>(t2)), std::forward<Args>(args)...));
 };

 template<typename T1, typename T2> struct apply_on_zip_impl<-1,T1,T2> {
  template<typename F, typename ... Args>
   auto operator()(F && f, T1 const & t1, T2 const & t2, Args && ... args) DECL_AND_RETURN( std::make_tuple(std::forward<Args>(args)...));
 };

 template<typename F, typename T1, typename T2>
  auto apply_on_zip (F && f,T1 const & t1, T2 const & t2) DECL_AND_RETURN( apply_on_zip_impl<std::tuple_size<T1>::value-1,T1,T2>()(std::forward<F>(f),t1,t2));

 /**
  * apply_on_tuple(f, t1,t2,t3)
  * f : a callable object
  * t1, t2 two tuples of the same size
  * Returns : [f(i,j) for i,j in zip(t1,t2)]
 */
 template<int pos, typename T1, typename T2, typename T3> struct apply_on_zip3_impl {
  template<typename F, typename ... Args>
   auto operator()(F && f, T1 const & t1, T2 const & t2, T3 const & t3, Args && ... args)
   DECL_AND_RETURN( apply_on_zip3_impl<pos-1,T1,T2,T3>()(std::forward<F>(f),t1, t2, t3, f(std::get<pos>(t1),std::get<pos>(t2),std::get<pos>(t3)), std::forward<Args>(args)...));
 };

 template<typename T1, typename T2, typename T3> struct apply_on_zip3_impl<-1,T1,T2,T3> {
  template<typename F, typename ... Args>
   auto operator()(F && f, T1 const & t1, T2 const & t2,  T3 const & t3, Args && ... args) DECL_AND_RETURN( std::make_tuple(std::forward<Args>(args)...));
 };

 template<typename F, typename T1, typename T2, typename T3>
  auto apply_on_zip3 (F && f,T1 const & t1, T2 const & t2, T3 const & t3) DECL_AND_RETURN( apply_on_zip3_impl<std::tuple_size<T1>::value-1,T1,T2,T3>()(std::forward<F>(f),t1,t2,t3));

 /**
  * fold(f, t1, init)
  * f : a callable object
  * t a tuple
  * Returns : f(x0,f(x1,f(....)) on the tuple
  */
 template<int pos, typename T> struct fold_impl {
  template<typename F, typename R>
   auto operator()(F && f, T & t, R && r )
   DECL_AND_RETURN( fold_impl<pos-1,T>()(std::forward<F>(f),t, f(std::get<pos>(t), std::forward<R>(r))));
 };

 template<typename T> struct fold_impl<-1,T> {
  template<typename F, typename R> R operator()(F && f, T & t, R && r) {return std::forward<R>(r);}
 };

 template<typename F, typename T, typename R>
  auto fold (F && f,T & t, R && r) DECL_AND_RETURN( fold_impl<std::tuple_size<T>::value-1,T>()(std::forward<F>(f),t,std::forward<R>(r)));
 template<typename F, typename T, typename R>
  auto fold (F && f,T const & t, R && r) DECL_AND_RETURN( fold_impl<std::tuple_size<T>::value-1,T const>()(std::forward<F>(f),t,std::forward<R>(r)));

 /**
  * fold_on_zip(f, t1, t2, init)
  * f : a callable object
  * t1, t2 two tuples of the same size
  * Returns : f(x0,y0,f(x1,y1,,f(....)) for t1 = (x0,x1 ...) and t2 = (y0,y1...).
  */
 template<int pos, typename T1, typename T2> struct fold_on_zip_impl {
  template<typename F, typename R>
   auto operator()(F && f, T1 const & t1, T2 const & t2, R && r )
   DECL_AND_RETURN( fold_on_zip_impl<pos-1,T1,T2>()(std::forward<F>(f),t1,t2, f(std::get<pos>(t1), std::get<pos>(t2), std::forward<R>(r))));
 };

 template<typename T1, typename T2> struct fold_on_zip_impl<-1,T1,T2> {
  template<typename F, typename R> R operator()(F && f, T1 const & t1, T2 const & t2, R && r) {return std::forward<R>(r);}
 };

 template<typename F, typename T1, typename T2, typename R>
  auto fold_on_zip (F && f,T1 const & t1, T2 const & t2, R && r) DECL_AND_RETURN( fold_on_zip_impl<std::tuple_size<T1>::value-1,T1,T2>()(std::forward<F>(f),t1,t2,std::forward<R>(r)));

}}
#endif
