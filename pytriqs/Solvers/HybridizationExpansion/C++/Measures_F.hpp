
/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2011 by M. Ferrero, O. Parcollet
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

#ifndef TRIQS_CTHYB1_MEASURES_F_H
#define TRIQS_CTHYB1_MEASURES_F_H

#include "Configuration.hpp"
#include "Measures_Z.hpp"
#include "gf_binner_and_eval.hpp"

/**
   Measure the F function (one bloc only) in time.
*/
class Measure_F_tau : public Measure_acc_sign<COMPLEX> {
  typedef Measure_acc_sign<COMPLEX> BaseType;
  const std::string name;
  const Configuration & Config;
  GF_Bloc_ImTime F_tau;
  gf_binner<GF_Bloc_ImTime> F_tau_bin;
  const int a_level;
public :   

  Measure_F_tau(const Configuration & Config_,int a, GF_Bloc_ImTime & Ftau_):
    BaseType(), name(to_string("G(tau)",a)), Config(Config_), F_tau(Ftau_), F_tau_bin(F_tau), a_level(a) { }

  void accumulate(COMPLEX signe) {
    BaseType::accumulate(signe);
    const double s(real(signe)); // not elegant !
    for (Configuration::DET_TYPE::C_Cdagger_M_iterator p(*Config.dets[a_level]); !p.atEnd(); ++p) {
      double r = Config.DT.traceRatioWithOneOperatorReplaced(p.C() , Config.H[p.C()->Op->name + "_Comm_Hloc"]);
      F_tau_bin(Config.info[p.C()->Op->Number].alpha, p.C()->tau, 
	      Config.info[p.Cdagger()->Op->Number].alpha, p.Cdagger()->tau, 
	      s * r * p.M());
    }
  }

  void collect_results(boost::mpi::communicator const & c){
    BaseType::collect_results(c);
    mc_weight_type Z_qmc ( this->acc_sign);
    F_tau.MPI_reduce_sum_onsite();
    F_tau.MPI_bcast();
    F_tau /= + real(Z_qmc) * Config.Beta* (Config.Beta/F_tau.numberTimeSlices);
  }

};

#endif
