// Copyright (C) 2004, International Business Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Andreas Waechter              IBM    2004-09-23

#include "IpOrigIterationOutput.hpp"
#ifdef OLD_C_HEADERS
# include <math.h>
#else
# include <cmath>
#endif

namespace Ipopt
{
  static const Index dbg_verbosity = 0;

  OrigIterationOutput::OrigIterationOutput()
  {}

  OrigIterationOutput::~OrigIterationOutput()
  {}

  bool OrigIterationOutput::InitializeImpl(const OptionsList& options,
      const std::string& prefix)
  {
    return true;
  }

  void OrigIterationOutput::WriteOutput()
  {
    //////////////////////////////////////////////////////////////////////
    //         First print the summary line for the iteration           //
    //////////////////////////////////////////////////////////////////////

    Index iter = IpData().iter_count();
    std::string header =
      " iter     objective    inf_pr   inf_du lg(mu)  ||d||  lg(rg) alpha_du alpha_pr  ls\n";
    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "\n\n**************************************************\n");
    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "*** Summary of Iteration: %d:", IpData().iter_count());
    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "\n**************************************************\n\n");
    if (iter%10 == 0 && !IpData().info_skip_output()) {
      // output the header
      Jnlst().Printf(J_SUMMARY, J_MAIN, header.c_str());
    }
    else {
      Jnlst().Printf(J_DETAILED, J_MAIN, header.c_str());
    }
    Number inf_pr = IpCq().curr_primal_infeasibility(IpoptCalculatedQuantities::NORM_MAX);
    Number inf_du = IpCq().curr_dual_infeasibility(IpoptCalculatedQuantities::NORM_MAX);
    Number mu = IpData().curr_mu();
    Number dnrm = Max(IpData().delta_x()->Amax(), IpData().delta_s()->Amax());
    Number f = IpCq().curr_f();

    // Retrieve some information set in the different parts of the algorithm
    char info_iter=' ';
    Number alpha_primal = IpData().info_alpha_primal();
    char alpha_primal_char = IpData().info_alpha_primal_char();
    Number alpha_dual = IpData().info_alpha_dual();
    Number regu_x = IpData().info_regu_x();
    char regu_x_buf[8];
    char dashes[]="   - ";
    char *regu_x_ptr;
    if (regu_x==.0) {
      regu_x_ptr = dashes;
    }
    else {
      sprintf(regu_x_buf, "%5.1f", log10(regu_x));
      regu_x_ptr = regu_x_buf;
    }
    Index ls_count = IpData().info_ls_count();
    const std::string info_string = IpData().info_string();

    if (!IpData().info_skip_output()) {
      Jnlst().Printf(J_SUMMARY, J_MAIN,
                     "%5d%c %14.7e %7.2e %7.2e %5.1f %7.2e %5s %7.2e %7.2e%c%3d %s\n",
                     iter, info_iter, f, inf_pr, inf_du, log10(mu), dnrm, regu_x_ptr,
                     alpha_dual, alpha_primal, alpha_primal_char,
                     ls_count, info_string.c_str());
    }


    //////////////////////////////////////////////////////////////////////
    //           Now if desired more detail on the iterates             //
    //////////////////////////////////////////////////////////////////////

    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "\n**************************************************\n");
    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "*** Beginning Iteration %d from the following point:",
                   IpData().iter_count());
    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "\n**************************************************\n\n");

    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_x", *IpData().curr_x());
    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "||curr_x||_inf = %.16e\n", IpData().curr_x()->Amax());
    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_s", *IpData().curr_s());
    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "||curr_s||_inf = %.16e\n", IpData().curr_s()->Amax());

    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_y_c", *IpData().curr_y_c());
    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_y_d", *IpData().curr_y_d());

    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_z_L", *IpData().curr_z_L());
    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_z_U", *IpData().curr_z_U());

    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_v_L", *IpData().curr_v_L());
    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_v_U", *IpData().curr_v_U());

    Jnlst().Printf(J_DETAILED, J_MAIN,
                   "\n\n***Current NLP Values for Iteration %d:\n",
                   IpData().iter_count());
    Jnlst().Printf(J_DETAILED, J_MAIN, "Objective = %.16e\n", IpCq().curr_f());
    Jnlst().PrintVector(J_VECTOR, J_MAIN, "grad_f", *IpCq().curr_grad_f());
    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_c", *IpCq().curr_c());
    Jnlst().PrintVector(J_VECTOR, J_MAIN, "curr_d", *IpCq().curr_d());
    Jnlst().PrintVector(J_VECTOR, J_MAIN,
                        "curr_d - curr_s", *IpCq().curr_d_minus_s());

    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_x", *IpData().delta_x());
    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_s", *IpData().delta_s());
    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_y_c", *IpData().delta_y_c());
    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_y_d", *IpData().delta_y_d());
    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_z_L", *IpData().delta_z_L());
    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_z_U", *IpData().delta_z_U());
    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_v_L", *IpData().delta_v_L());
    //     Jnlst().PrintVector(J_MOREVECTOR, J_MAIN, "delta_v_U", *IpData().delta_v_U());

    Jnlst().PrintMatrix(J_MATRIX, J_MAIN, "jac_c", *IpCq().curr_jac_c());
    Jnlst().PrintMatrix(J_MATRIX, J_MAIN, "jac_d", *IpCq().curr_jac_d());
    Jnlst().PrintMatrix(J_MATRIX, J_MAIN, "h", *IpCq().curr_exact_hessian());
    Jnlst().Printf(J_DETAILED, J_MAIN, "\n\n");

  }

} // namespace Ipopt