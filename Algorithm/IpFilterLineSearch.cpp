// Copyright (C) 2004, International Business Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-08-13

#include "IpFilterLineSearch.hpp"
#include "IpJournalist.hpp"
#include "IpRestoPhase.hpp"

#ifdef OLD_C_HEADERS
# include <math.h>
# include <ctype.h>
#else
# include <cmath>
# include <cctype>
#endif

namespace Ipopt
{

  static const Index dbg_verbosity = 0;

  FilterLineSearch::FilterLineSearch(const SmartPtr<RestorationPhase>& resto_phase,
                                     const SmartPtr<PDSystemSolver>& pd_solver)
      :
      LineSearch(),
      resto_phase_(resto_phase),
      pd_solver_(pd_solver),
      filter_size_(0),
      theta_min_(-1.0),
      theta_max_(-1.0),
      filter_()
  {}

  FilterLineSearch::~FilterLineSearch()
  {}

  bool FilterLineSearch::InitializeImpl(const OptionsList& options,
                                        const std::string& prefix)
  {
    Number value = 0.0;
    Int ivalue = 0;

    // Check for the algorithm options
    if (options.GetNumericValue("theta_max_fact", value, prefix)) {
      ASSERT_EXCEPTION(value > 0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"theta_max_fact\": This value must be larger than 0.");
      theta_max_fact_ = value;
    }
    else {
      theta_max_fact_ = 1e4;
    }

    if (options.GetNumericValue("theta_min_fact", value, prefix)) {
      ASSERT_EXCEPTION(value > 0 && value < theta_max_fact_, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"theta_min_fact\": This value must be larger than 0 and less than theta_max_fact.");
      theta_min_fact_ = value;
    }
    else {
      theta_min_fact_ = 1e-4;
    }

    if (options.GetNumericValue("eta_phi", value, prefix)) {
      ASSERT_EXCEPTION(value > 0 && value < 0.5, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"eta_phi\": This value must be between 0 and 0.5.");
      eta_phi_ = value;
    }
    else {
      eta_phi_ = 1e-4;
    }

    if (options.GetNumericValue("delta", value, prefix)) {
      ASSERT_EXCEPTION(value > 0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"delta\": This value must be larger than 0.");
      delta_ = value;
    }
    else {
      delta_ = 1.0;
    }

    if (options.GetNumericValue("s_phi", value, prefix)) {
      ASSERT_EXCEPTION(value > 1.0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"s_phi\": This value must be larger than 1.");
      s_phi_ = value;
    }
    else {
      s_phi_ = 2.3;
    }

    if (options.GetNumericValue("s_theta", value, prefix)) {
      ASSERT_EXCEPTION(value > 1.0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"s_theta\": This value must be larger than 1.0.");
      s_theta_ = value;
    }
    else {
      s_theta_ = 1.1;
    }


    if (options.GetNumericValue("gamma_phi", value, prefix)) {
      ASSERT_EXCEPTION(value > 0.0 && value < 1.0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"gamma_phi\": This value must be between 0 and 1.");
      gamma_phi_ = value;
    }
    else {
      gamma_phi_ = 1e-5;
    }

    if (options.GetNumericValue("gamma_theta", value, prefix)) {
      ASSERT_EXCEPTION(value > 0.0 && value < 1.0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"gamma_theta\": This value must be between 0 and 1.");
      gamma_theta_ = value;
    }
    else {
      gamma_theta_ = 1e-5;
    }

    if (options.GetNumericValue("alpha_min_frac", value, prefix)) {
      ASSERT_EXCEPTION(value > 0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"alpha_min_frac\": This value must be > 0 and <= 1.");
      alpha_min_frac_ = value;
    }
    else {
      alpha_min_frac_ = 0.05;
    }

    if (options.GetNumericValue("alpha_red_factor", value, prefix)) {
      ASSERT_EXCEPTION(value > 0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"alpha_red_factor\": This value must be larger than 0.");
      alpha_red_factor_ = value;
    }
    else {
      alpha_red_factor_ = 0.5;
    }

    if (options.GetIntegerValue("max_soc", ivalue, prefix)) {
      ASSERT_EXCEPTION(ivalue >= 0, OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"max_soc\": This value must be non-negative.");
      max_soc_ = ivalue;
    }
    else {
      max_soc_ = 4;
    }
    if (max_soc_>0) {
      ASSERT_EXCEPTION(IsValid(pd_solver_), OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"max_soc\": This option is non-negative, but no linear solver for computing the SOC given to FilterLineSearch object.");
    }

    if (options.GetNumericValue("kappa_soc", value, prefix)) {
      ASSERT_EXCEPTION(value > 0., OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"kappa_soc\": This value must be larger than 0.");
      kappa_soc_ = value;
    }
    else {
      kappa_soc_ = 0.99;
    }

    if (options.GetNumericValue("obj_max_inc_", value, prefix)) {
      ASSERT_EXCEPTION(value > 1., OptionsList::OPTION_OUT_OF_RANGE,
                       "Option \"obj_max_inc_\": This value must be larger than 1.");
      obj_max_inc_ = value;
    }
    else {
      obj_max_inc_ = 5;
    }

    if (options.GetIntegerValue("magic_steps", ivalue, prefix)) {
      magic_steps_ = (ivalue != 0);
    }
    else {
      magic_steps_ = false;
    }

    bool retvalue = true;
    if (IsValid(resto_phase_)) {
      retvalue = resto_phase_->Initialize(Jnlst(), IpNLP(), IpData(), IpCq(),
                                          options, prefix);
    }

    // ToDo decide if also the PDSystemSolver should be initialized here...

    return retvalue;
  }

  void FilterLineSearch::FindAcceptableTrialPoint()
  {
    DBG_START_METH("FilterLineSearch::FindAcceptableTrialPoint",
                   dbg_verbosity);
    Jnlst().Printf(J_DETAILED, J_LINE_SEARCH,
                   "--> Starting filter line search in iteration %d <--\n",
                   IpData().iter_count());

    // Get the search directions (if an SOC step is accepted, those poiters
    // will later point to the SOC search direction)
    SmartPtr<const Vector> delta_x = IpData().delta_x();
    SmartPtr<const Vector> delta_s = IpData().delta_s();
    SmartPtr<const Vector> delta_y_c = IpData().delta_y_c();
    SmartPtr<const Vector> delta_y_d = IpData().delta_y_d();
    SmartPtr<const Vector> delta_z_L = IpData().delta_z_L();
    SmartPtr<const Vector> delta_z_U = IpData().delta_z_U();
    SmartPtr<const Vector> delta_v_L = IpData().delta_v_L();
    SmartPtr<const Vector> delta_v_U = IpData().delta_v_U();

    // Step size used in ftype and armijo tests
    Number alpha_primal_test;

    // Compute smallest step size allowed
    Number alpha_min = CalculateAlphaMin();
    Jnlst().Printf(J_DETAILED, J_LINE_SEARCH,
                   "minimal step size ALPHA_MIN = %E\n", alpha_min);

    // Start line search from primal fraction-to-the-boundary value
    Number alpha_primal_max =
      IpCq().curr_primal_frac_to_the_bound(IpData().curr_tau());
    Number alpha_primal = alpha_primal_max;

    filter_.Print(Jnlst());

    // Loop over decreaseing step sizes until acceptable point is found or
    // until step size becomes too small
    bool accept = false;
    bool soc_taken = false;
    Index n_steps = 0;

    while (alpha_primal>alpha_min || n_steps == 0) { // always allow the "full" step if it is acceptable (even if alpha_primal<=alpha_min)
      Jnlst().Printf(J_DETAILED, J_LINE_SEARCH,
                     "Starting checks for alpha (primal) = %lf\n",
                     alpha_primal);

      alpha_primal_test = alpha_primal;
      try {
        // Compute the primal trial point
        IpData().SetTrialPrimalVariablesFromStep(alpha_primal,
            *delta_x, *delta_s);

        if (magic_steps_) {
          PerformMagicStep();
        }

        // If it is acceptable, stop the search
        accept = CheckAcceptabilityOfTrialPoint(alpha_primal_test);
      }
      catch(IpoptNLP::Eval_Error& e) {
        e.ReportException(Jnlst());
        Jnlst().Printf(J_WARNING, J_MAIN,
                       "Warning: Cutting back alpha due to evaluation error\n");
        accept = false;
      }

      if (accept) {
        break;
      }

      // Try second order correction
      Number theta_curr = IpCq().curr_constraint_violation();
      Number theta_trial = IpCq().trial_constraint_violation();
      if (alpha_primal==alpha_primal_max &&       // i.e. first trial point
          theta_curr<=theta_trial && max_soc_>0) {
        Index count_soc = 0;
        Number theta_soc_old = 0.;
        theta_trial = 0.;
        Number alpha_primal_soc = alpha_primal;

        SmartPtr<Vector> c_soc = IpCq().curr_c()->MakeNew();
        SmartPtr<Vector> dms_soc = IpCq().curr_d_minus_s()->MakeNew();
        c_soc->Copy(*IpCq().curr_c());
        dms_soc->Copy(*IpCq().curr_d_minus_s());
        while (count_soc<max_soc_ &&
               theta_trial<=kappa_soc_*theta_soc_old &&
               !accept) {
          if (count_soc==0) {
            theta_soc_old = theta_curr;
          }
          else {
            theta_soc_old = theta_trial;
          }

          Jnlst().Printf(J_DETAILED, J_LINE_SEARCH,
                         "Trying second order correction number %d\n",
                         count_soc+1);

          // Compute SOC constraint violation
          c_soc->Scal(alpha_primal_soc);
          dms_soc->Scal(alpha_primal_soc);
          c_soc->Axpy(1.0, *IpCq().trial_c());
          dms_soc->Axpy(1.0, *IpCq().trial_d_minus_s());

          // Compute the SOC search direction
          SmartPtr<Vector> delta_soc_x = delta_x->MakeNew();
          SmartPtr<Vector> delta_soc_s = delta_s->MakeNew();
          SmartPtr<Vector> delta_soc_y_c = delta_y_c->MakeNew();
          SmartPtr<Vector> delta_soc_y_d = delta_y_d->MakeNew();
          SmartPtr<Vector> delta_soc_z_L = delta_z_L->MakeNew();
          SmartPtr<Vector> delta_soc_z_U = delta_z_U->MakeNew();
          SmartPtr<Vector> delta_soc_v_L = delta_v_L->MakeNew();
          SmartPtr<Vector> delta_soc_v_U = delta_v_U->MakeNew();
          ComputeSecondOrderSearchDirection(*c_soc, *dms_soc,
                                            *delta_soc_x, *delta_soc_s,
                                            *delta_soc_y_c, *delta_soc_y_d,
                                            *delta_soc_z_L, *delta_soc_z_U,
                                            *delta_soc_v_L, *delta_soc_v_U);

          // Compute step size
          alpha_primal_soc =
            IpCq().primal_frac_to_the_bound(IpData().curr_tau(),
                                            *delta_soc_x,
                                            *delta_soc_s);

          // Check if trial point is acceptable
          try {
            // Compute the primal trial point
            IpData().SetTrialPrimalVariablesFromStep(alpha_primal_soc,
                *delta_soc_x,
                *delta_soc_s);

            // in acceptance tests, use original step size!
            accept = CheckAcceptabilityOfTrialPoint(alpha_primal_test);
          }
          catch(IpoptNLP::Eval_Error& e) {
            e.ReportException(Jnlst());
            Jnlst().Printf(J_WARNING, J_MAIN, "Warning: SOC step rejected due to evaluation error\n");
            accept = false;
          }

          if (accept) {
            Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Second order correction step accepted with %d corrections.\n", count_soc+1);
            // Accept all SOC quantities
            alpha_primal = alpha_primal_soc;
            delta_x = ConstPtr(delta_soc_x);
            delta_s = ConstPtr(delta_soc_s);
            delta_y_c = ConstPtr(delta_soc_y_c);
            delta_y_d = ConstPtr(delta_soc_y_d);
            delta_z_L = ConstPtr(delta_soc_z_L);
            delta_z_U = ConstPtr(delta_soc_z_U);
            delta_v_L = ConstPtr(delta_soc_v_L);
            delta_v_U = ConstPtr(delta_soc_v_U);
            soc_taken = true;
          }
          else {
            count_soc++;
            theta_trial = IpCq().trial_constraint_violation();
          }
        }

        if (accept) {
          break;
        }
      }

      // Point is not yet acceptable, try a shorter one
      alpha_primal *= alpha_red_factor_;
      n_steps++;
    }

    // If line search has been aborted because the step size becomes too small,
    // go to the restoration phase
    if (!accept) {
      if (IsValid(resto_phase_)) {
        // Set the info fields for the first output line in the
        // restoration phase which reflects why the restoration phase
        // was called
        IpData().Set_info_alpha_primal(alpha_primal);
        IpData().Set_info_alpha_dual(0.);
        IpData().Set_info_alpha_primal_char('R');
        IpData().Set_info_ls_count(n_steps+1);

        accept = resto_phase_->PerformRestoration();
      }
      else {
        //ToDo
        DBG_ASSERT(false && "No Restoration Phase given to this Filter Line Search Object!");
      }
    }

    if (!accept) {
      DBG_ASSERT(false && "Failed restoration phase!!!");
    }

    char info_alpha_primal_char;
    // Augment the filter if required
    if (!IsFtype(alpha_primal_test) || !ArmijoHolds(alpha_primal_test)) {
      AugmentFilter();
      info_alpha_primal_char = 'h';
    }
    else {
      info_alpha_primal_char = 'f';
    }
    if (soc_taken) {
      info_alpha_primal_char = toupper(info_alpha_primal_char);
    }
    IpData().Set_info_alpha_primal_char(info_alpha_primal_char);
    IpData().Set_info_ls_count(n_steps+1);

    // Print the current filter
    // ToDo do that call only for high enough debug level?
    //    filter_.Print(jnlst);

    // Now compute values of the remaining variables into the trial points
    IpData().SetTrialEqMultipilersFromStep(alpha_primal,
                                           *delta_y_c, *delta_y_d);
    Number alpha_dual_max =
      IpCq().dual_frac_to_the_bound(IpData().curr_tau(),
                                    *delta_z_L, *delta_z_U,
                                    *delta_v_L, *delta_v_U);

    IpData().SetTrialBoundMutlipliersFromStep(alpha_dual_max,
        *delta_z_L, *delta_z_U,
        *delta_v_L, *delta_v_U);

    // Set some information for iteration summary output
    IpData().Set_info_alpha_primal(alpha_primal);
    IpData().Set_info_alpha_dual(alpha_dual_max);

  }

  bool FilterLineSearch::IsFtype(Number alpha_primal_test)
  {
    DBG_START_METH("FilterLineSearch::IsFtype",
                   dbg_verbosity);
    Number curr_theta = IpCq().curr_constraint_violation();

    return (IpCq().curr_gradBarrTDelta() < 0.0 &&
            alpha_primal_test*pow(-IpCq().curr_gradBarrTDelta(),s_phi_) >
            delta_*pow(curr_theta,s_theta_));
  }

  void FilterLineSearch::AugmentFilter()
  {
    DBG_START_METH("FilterLineSearch::AugmentFilter",
                   dbg_verbosity);
    Number curr_barr = IpCq().curr_barrier_obj();
    Number curr_theta = IpCq().curr_constraint_violation();

    Number phi_add = curr_barr - gamma_phi_*curr_theta;
    Number theta_add = (1.-gamma_theta_)*curr_theta;

    filter_.AddEntry(phi_add, theta_add, IpData().iter_count());
  }

  bool
  FilterLineSearch::CheckAcceptabilityOfTrialPoint(Number alpha_primal_test)
  {
    DBG_START_METH("FilterLineSearch::CheckAcceptabilityOfTrialPoint",
                   dbg_verbosity);

    bool accept;

    //ToDo:  Here will be a catch of exceptions for problems during the
    //       funciton evaluations

    // First compute the barrier function and constraint violation at the
    // current iterate and the trial point
    Number curr_barr = IpCq().curr_barrier_obj();
    Number curr_theta = IpCq().curr_constraint_violation();

    Number trial_theta = IpCq().trial_constraint_violation();
    // Check if constraint violation is becoming too large
    if (theta_max_ < 0.0) {
      theta_max_ = theta_max_fact_*Max(1.0, curr_theta);
    }
    if (theta_min_ < 0.0) {
      theta_min_ = theta_min_fact_*Max(1.0, curr_theta);
    }

    if (theta_max_>0 && trial_theta>theta_max_) {
      return false;
    }

    Number trial_barr = IpCq().trial_barrier_obj();

    Jnlst().Printf(J_DETAILED, J_LINE_SEARCH,
                   "Checking acceptability for trial step size alpha_primal_test=%13.6e:\n", alpha_primal_test);
    Jnlst().Printf(J_DETAILED, J_LINE_SEARCH,
                   "  New values of barrier function     = %23.16e  (current %23.16e):\n", trial_barr, curr_barr);
    Jnlst().Printf(J_DETAILED, J_LINE_SEARCH,
                   "  New values of constraint violation = %23.16e  (current %23.16e):\n", trial_theta, curr_theta);

    // Check if point is acceptable w.r.t current iterate
    if (IsFtype(alpha_primal_test) && curr_theta <= theta_min_) {
      // Armijo condition for the barrier function has to be satisfied
      Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Checking Armijo Condition...\n");
      accept = ArmijoHolds(alpha_primal_test);
    }
    else {
      Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Checking sufficient reduction...\n");
      accept = IsAcceptableToCurrentIterate(trial_barr, trial_theta);
    }

    if (!accept) {
      Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Failed...\n");
      return accept;
    }
    else {
      Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Succeeded...\n");
    }

    // Now check if that pair is acceptable to the filter
    Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Checking filter acceptability...\n");
    accept = IsAcceptableToCurrentFilter(trial_barr, trial_theta);
    if (!accept) {
      Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Failed...\n");
      return accept;
    }
    else {
      Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Succeeded...\n");
    }

    return accept;
  }

  bool FilterLineSearch::ArmijoHolds(Number alpha_primal_test)
  {
    return Compare_le(IpCq().trial_barrier_obj()-IpCq().curr_barrier_obj(),
                      eta_phi_*alpha_primal_test*IpCq().curr_gradBarrTDelta(),
                      IpCq().curr_barrier_obj());
  }

  Number FilterLineSearch::CalculateAlphaMin()
  {
    Number gBD = IpCq().curr_gradBarrTDelta();
    Number curr_theta = IpCq().curr_constraint_violation();
    Number alpha_min = gamma_theta_;

    if (gBD < 0) {
      alpha_min = Min( gamma_theta_,
                       gamma_phi_*curr_theta/(-gBD));
      if (curr_theta <= theta_min_) {
        alpha_min = Min( alpha_min,
                         delta_*pow(curr_theta,s_theta_)/pow(-gBD,s_phi_)
                       );
      }
    }

    return alpha_min_frac_*alpha_min;
  }

  bool FilterLineSearch::IsAcceptableToCurrentIterate(Number trial_barr, Number trial_theta) const
  {
    DBG_START_METH("FilterLineSearch::IsAcceptableToCurrentIterate",
                   dbg_verbosity);
    Number curr_barr = IpCq().curr_barrier_obj();

    // Check if the barrier objective function is increasing to
    // rapidly (according to option obj_max_inc)
    if (trial_barr > curr_barr) {
      Number basval = 1.;
      if (fabs(curr_barr)>10.) {
        basval = log10(fabs(curr_barr));
      }
      if (log10(trial_barr-curr_barr)>obj_max_inc_*basval) {
        Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Rejecting trial point because barrier objective function increasing too rapidly (from %27.15e to %27.15e)\n",curr_barr,trial_barr);
        return false;
      }
    }

    Number curr_theta = IpCq().curr_constraint_violation();
    DBG_PRINT((1,"trial_barr  = %e curr_barr  = %e\n", trial_barr, curr_barr));
    DBG_PRINT((1,"trial_theta = %e curr_theta = %e\n", trial_theta, curr_theta));
    return (Compare_le(trial_theta, (1.-gamma_theta_)*curr_theta, curr_theta)
            || Compare_le(trial_barr-curr_barr, -gamma_phi_*curr_theta, curr_barr));
  }

  bool FilterLineSearch::IsAcceptableToCurrentFilter(Number trial_barr, Number trial_theta) const
  {
    return filter_.Acceptable(trial_barr, trial_theta);
  }

  bool FilterLineSearch::Compare_le(Number lhs, Number rhs, Number BasVal)
  {
    DBG_START_FUN("FilterLineSearch::Compare_le",
                  dbg_verbosity);
    // ToDo: Comparison based on machine precision
    return (lhs - rhs <= 1e-15*fabs(BasVal));
  }

  void FilterLineSearch::Reset()
  {
    DBG_START_FUN("FilterLineSearch::Reset", dbg_verbosity);
    filter_.Clear();
  }

  void FilterLineSearch::ComputeSecondOrderSearchDirection(const Vector& c_soc,
      const Vector& d_minus_s_soc,
      Vector& delta_soc_x,
      Vector& delta_soc_s,
      Vector& delta_soc_y_c,
      Vector& delta_soc_y_d,
      Vector& delta_soc_z_L,
      Vector& delta_soc_z_U,
      Vector& delta_soc_v_L,
      Vector& delta_soc_v_U)
  {
    SmartPtr<const Vector> rhs_grad_lag_x  = IpCq().curr_grad_lag_x();
    SmartPtr<const Vector> rhs_grad_lag_s  = IpCq().curr_grad_lag_s();
    SmartPtr<const Vector> rhs_rel_compl_x_L = IpCq().curr_relaxed_compl_x_L();
    SmartPtr<const Vector> rhs_rel_compl_x_U = IpCq().curr_relaxed_compl_x_U();
    SmartPtr<const Vector> rhs_rel_compl_s_L = IpCq().curr_relaxed_compl_s_L();
    SmartPtr<const Vector> rhs_rel_compl_s_U = IpCq().curr_relaxed_compl_s_U();
    pd_solver_->Solve(-1.0, 0.0,
                      *rhs_grad_lag_x,
                      *rhs_grad_lag_s,
                      c_soc,
                      d_minus_s_soc,
                      *rhs_rel_compl_x_L,
                      *rhs_rel_compl_x_U,
                      *rhs_rel_compl_s_L,
                      *rhs_rel_compl_s_U,
                      delta_soc_x,
                      delta_soc_s,
                      delta_soc_y_c,
                      delta_soc_y_d,
                      delta_soc_z_L,
                      delta_soc_z_U,
                      delta_soc_v_L,
                      delta_soc_v_U,
                      true);

  }

  void
  FilterLineSearch::PerformMagicStep()
  {
    DBG_START_METH("FilterLineSearch::PerformMagicStep",
                   0);

    DBG_PRINT((1,"Incoming barr = %e and constrviol %e\n", IpCq().trial_barrier_obj(), IpCq().trial_constraint_violation()));
    DBG_PRINT_VECTOR(2, "s in", *IpData().trial_s());
    DBG_PRINT_VECTOR(2, "d minus s in", *IpCq().trial_d_minus_s());
    DBG_PRINT_VECTOR(2, "slack_s_L in", *IpCq().trial_slack_s_L());
    DBG_PRINT_VECTOR(2, "slack_s_U in", *IpCq().trial_slack_s_U());

    SmartPtr<const Vector> d_L = IpNLP().d_L();
    SmartPtr<const Matrix> Pd_L = IpNLP().Pd_L();
    SmartPtr<Vector> delta_s_magic_L = d_L->MakeNew();
    delta_s_magic_L->Set(0.);
    SmartPtr<Vector> tmp = d_L->MakeNew();
    Pd_L->TransMultVector(1., *IpCq().trial_d_minus_s(), 0., *tmp);
    delta_s_magic_L->ElementWiseMax(*tmp);

    SmartPtr<const Vector> d_U = IpNLP().d_U();
    SmartPtr<const Matrix> Pd_U = IpNLP().Pd_U();
    SmartPtr<Vector> delta_s_magic_U = d_U->MakeNew();
    delta_s_magic_U->Set(0.);
    tmp = d_U->MakeNew();
    Pd_U->TransMultVector(1., *IpCq().trial_d_minus_s(), 0., *tmp);
    delta_s_magic_U->ElementWiseMin(*tmp);

    SmartPtr<Vector> delta_s_magic = IpData().trial_s()->MakeNew();
    Pd_L->MultVector(1., *delta_s_magic_L, 0., *delta_s_magic);
    Pd_U->MultVector(1., *delta_s_magic_U, 1., *delta_s_magic);
    delta_s_magic_L = NULL; // free memory
    delta_s_magic_U = NULL; // free memory

    // Now find those entries with both lower and upper bounds, there
    // the step is too large
    // ToDo this should only be done if there are inequality
    // constraints with two bounds
    // also this can be done in a smaller space (d_L or d_U whichever
    // is smaller)
    tmp = delta_s_magic->MakeNew();
    tmp->Copy(*IpData().trial_s());
    Pd_L->MultVector(1., *d_L, -2., *tmp);
    Pd_U->MultVector(1., *d_U, 1., *tmp);
    SmartPtr<Vector> tmp2 = tmp->MakeNew();
    tmp2->Copy(*tmp);
    tmp2->ElementWiseAbs();
    tmp->Axpy(-2., *delta_s_magic);
    tmp->ElementWiseAbs();
    // now, tmp2 = |d_L + d_u - 2*s| and tmp = |d_L + d_u - 2*(s+Delta s)|
    // we want to throw out those for which tmp2 > tmp
    tmp->Axpy(-1., *tmp2);
    tmp->ElementWiseSgn();
    tmp2->Set(0.);
    tmp2->ElementWiseMax(*tmp);
    tmp = d_L->MakeNew();
    Pd_L->TransMultVector(1., *tmp2, 0., *tmp);
    Pd_L->MultVector(1., *tmp, 0., *tmp2);
    tmp = d_U->MakeNew();
    Pd_U->TransMultVector(1., *tmp2, 0., *tmp);
    Pd_U->MultVector(1., *tmp, 0., *tmp2);
    DBG_PRINT_VECTOR(2, "tmp indicator", *tmp2)
    // tmp2 now is one for those entries with both bounds, for which
    // no step should be taken

    tmp = delta_s_magic->MakeNew();
    tmp->Copy(*delta_s_magic);
    tmp->ElementWiseMultiply(*tmp2);
    delta_s_magic->Axpy(-1., *tmp);

    Number delta_s_magic_max = delta_s_magic->Amax();
    Number mach_eps = std::numeric_limits<Number>::epsilon();
    if (delta_s_magic_max>0.) {
      if (delta_s_magic_max > 10*mach_eps*IpData().trial_s()->Amax()) {
        IpData().Append_info_string("M");
        Jnlst().Printf(J_DETAILED, J_LINE_SEARCH, "Magic step with max-norm %.6e taken.\n", delta_s_magic->Amax());
        Jnlst().PrintVector(J_MOREVECTOR, J_LINE_SEARCH,
                            "delta_s_magic", *delta_s_magic);
      }

      // now finally compute the new overall slacks
      delta_s_magic->Axpy(1., *IpData().trial_s());
      IpData().SetTrialSVariables(*delta_s_magic);
    }

    DBG_PRINT((1,"Outgoing barr = %e and constrviol %e\n", IpCq().trial_barrier_obj(), IpCq().trial_constraint_violation()));
    DBG_PRINT_VECTOR(2, "s out", *IpData().trial_s());
    DBG_PRINT_VECTOR(2, "d minus s out", *IpCq().trial_d_minus_s());
    DBG_PRINT_VECTOR(2, "slack_s_L out", *IpCq().trial_slack_s_L());
    DBG_PRINT_VECTOR(2, "slack_s_U out", *IpCq().trial_slack_s_U());
  }

  ///////////////////////////////////////////////////////////////////////////
  //                            Filter entries                             //
  ///////////////////////////////////////////////////////////////////////////

  FilterLineSearch::FilterEntry::FilterEntry(Number phi,
      Number theta, Index iter)
      :
      phi_(phi),
      theta_(theta),
      iter_(iter)
  {}

  FilterLineSearch::FilterEntry::~FilterEntry()
  {}

  ///////////////////////////////////////////////////////////////////////////
  //                                 Filter                                //
  ///////////////////////////////////////////////////////////////////////////

  bool FilterLineSearch::Filter::Acceptable(Number phi, Number theta) const
  {
    DBG_START_METH("FilterLineSearch::Filter::Acceptable", dbg_verbosity);
    bool acceptable = true;
    std::list<FilterEntry*>::iterator iter;
    for (iter = filter_list_.begin(); iter != filter_list_.end();
         iter++) {
      if (!(*iter)->Acceptable(phi, theta)) {
        acceptable = false;
        break;
      }
    }
    return acceptable;
  }

  void FilterLineSearch::Filter::AddEntry(Number phi, Number theta, Index iteration)
  {
    DBG_START_METH("FilterLineSearch::Filter::AddEntry", dbg_verbosity);
    std::list<FilterEntry*>::iterator iter;
    for (iter = filter_list_.begin(); iter != filter_list_.end();
         iter++) {
      if ((*iter)->Dominated(phi, theta)) {
        std::list<FilterEntry*>::iterator iter_to_remove = iter;
        iter--;
        FilterEntry* entry_to_remove = *iter_to_remove;
        filter_list_.erase(iter_to_remove);
        delete entry_to_remove;
      }
    }
    FilterEntry* new_entry = new FilterEntry(phi, theta, iteration);
    filter_list_.push_back(new_entry);
  }

  void FilterLineSearch::Filter::Clear()
  {
    DBG_START_METH("FilterLineSearch::Filter::Clear", dbg_verbosity);
    while (!filter_list_.empty()) {
      FilterEntry* entry = filter_list_.back();
      filter_list_.pop_back();
      delete entry;
    }
  }

  void FilterLineSearch::Filter::Print(const Journalist& jnlst)
  {
    DBG_START_METH("FilterLineSearch::Filter::Print", dbg_verbosity);
    jnlst.Printf(J_DETAILED, J_LINE_SEARCH,
                 "The current filter has %d entries.\n", filter_list_.size());
    std::list<FilterEntry*>::iterator iter;
    Index count = 0;
    for (iter = filter_list_.begin(); iter != filter_list_.end();
         iter++) {
      if (count % 10 == 0) {
        jnlst.Printf(J_VECTOR, J_LINE_SEARCH,
                     "                phi                    theta            iter\n");
      }
      count++;
      jnlst.Printf(J_VECTOR, J_LINE_SEARCH,
                   "%5d %23.16e %23.16e %5d\n",
                   count, (*iter)->phi(), (*iter)->theta(),
                   (*iter)->iter());
    }
  }

} // namespace Ipopt