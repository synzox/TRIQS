/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2013 by M. Ferrero, O. Parcollet, I. Krivenko
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
#ifndef TRIQS_CTQMC_KRYLOV_TIME_PT_HPP
#define TRIQS_CTQMC_KRYLOV_TIME_PT_HPP

#include <triqs/utility/first_include.hpp>
#include <limits>
#include <iostream>

namespace triqs { namespace utility {

 /// A point on a very thin grid, as uint64_t
 struct time_pt {

  time_pt() { beta = 1; val =0; n = 0;}
  time_pt(double v, double beta_) { beta = beta_; val =v; n = floor(Nmax*(v/beta));}
  time_pt(uint64_t n_, double beta_, bool) { beta = beta_; val = beta*(double(n_)/Nmax); n = n_;}

  time_pt (time_pt const &) = default;
  time_pt (time_pt && x) = default;
  time_pt & operator = (time_pt const &) = default ;
#ifndef TRIQS_WORKAROUND_INTEL_COMPILER_BUGS
  time_pt & operator = (time_pt && x) = default;
#else
  time_pt & operator = (time_pt && x) noexcept { using std::swap; swap(n,x.n); swap(beta, x.beta); swap(val, x.val); return *this;}
#endif

  time_pt & operator = (double v) { val =v; n = floor(Nmax*(v/beta)); return *this; }

  bool operator == (const time_pt & tp) const { return n == tp.n; }
  bool operator <  (const time_pt & tp) const { return n < tp.n; }
  bool operator >  (const time_pt & tp) const { return n > tp.n; }

  // adding and substracting is cyclic on [0, beta]
  inline friend time_pt operator+(time_pt const & a, time_pt const & b) { return time_pt(a.n + b.n, a.beta, true); }
  inline friend time_pt operator-(time_pt const & a, time_pt const & b) { uint64_t n = (a.n>= b.n ? a.n - b.n : Nmax - (b.n - a.n)); return time_pt(n, a.beta,true); }
  inline friend time_pt operator-(time_pt const & a) { uint64_t n = Nmax - a.n; return time_pt(n, a.beta,true); }
  
  operator double() const {return val;} // cast to a double

  friend std::ostream & operator<< (std::ostream & out, time_pt const & p) { return out << p.val << " [time_pt : beta = "<< p.beta<< " n = "<< p.n<<"]" ; }
 
  static time_pt epsilon(double beta) { return time_pt(1,beta,true);}

  private:
  static constexpr uint64_t Nmax = std::numeric_limits<uint64_t>::max();
  uint64_t n;
  double val, beta;
 };

  // all other operations : first cast into a double and do the operation
#define IMPL_OP(OP) \
  template<typename T> auto operator OP(time_pt const & x, T y) -> decltype(double(0) OP y) {return static_cast<double>(x) OP y;} \
  template<typename T> auto operator OP(T y, time_pt const & x) -> decltype(y OP double(0)) {return y OP static_cast<double>(x);} \
  IMPL_OP(+); IMPL_OP(-); IMPL_OP(*); IMPL_OP(/);
#undef IMPL_OP

}}
#endif
