// Copyright (C) 2004, International Business Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-09-02

#include "IpStdInterfaceTNLP.hpp"

namespace Ipopt
{
  StdInterfaceTNLP::StdInterfaceTNLP(const SmartPtr<const Journalist>& jnlst,
                                     Index n_var,
                                     const Number* x_L, const Number* x_U,
                                     Index n_con,
                                     const Number* g_L, const Number* g_U,
                                     Index nele_jac,
                                     Index nele_hess,
                                     const Number* start_x,
                                     const Number* start_lam,
                                     const Number* start_z_L,
                                     const Number* start_z_U,
                                     Eval_F_CB eval_f,
                                     Eval_G_CB eval_g,
                                     Eval_Grad_F_CB eval_grad_f,
                                     Eval_Jac_G_CB eval_jac_g,
                                     Eval_H_CB eval_h,
                                     UserDataPtr user_data)
      :
      TNLP(),
      jnlst_(jnlst),
      n_var_(n_var),
      n_con_(n_con),
      x_L_(x_L),
      x_U_(x_U),
      g_L_(g_L),
      g_U_(g_U),
      nele_jac_(nele_jac),
      nele_hess_(nele_hess),
      start_x_(start_x),
      start_lam_(start_lam),
      start_z_L_(start_z_L),
      start_z_U_(start_z_U),
      eval_f_(eval_f),
      eval_g_(eval_g),
      eval_grad_f_(eval_grad_f),
      eval_jac_g_(eval_jac_g),
      eval_h_(eval_h),
      user_data_(user_data),
      non_const_x_(NULL)
  {
    ASSERT_EXCEPTION(n_var_>0, INVALID_STDINTERFACE_NLP,
                     "The number of variables must be at least 1.");
    ASSERT_EXCEPTION(n_con_>=0, INVALID_STDINTERFACE_NLP,
                     "The number of constrains must be non-negative.");
    ASSERT_EXCEPTION(x_L_, INVALID_STDINTERFACE_NLP,
                     "No lower bounds for variables provided.");
    ASSERT_EXCEPTION(x_U_, INVALID_STDINTERFACE_NLP,
                     "No upper bounds for variables provided.");
    ASSERT_EXCEPTION(g_L_, INVALID_STDINTERFACE_NLP,
                     "No lower bounds for constraints provided.");
    ASSERT_EXCEPTION(g_U_, INVALID_STDINTERFACE_NLP,
                     "No upper bounds for constraints provided.");
    ASSERT_EXCEPTION(nele_jac_>=0, INVALID_STDINTERFACE_NLP,
                     "Number of non-zero elements in constraint Jacobian must be non-negative.");
    ASSERT_EXCEPTION(nele_hess_>=0, INVALID_STDINTERFACE_NLP,
                     "Number of non-zero elements in Hessian of Lagrangian must be non-negative.");
    ASSERT_EXCEPTION(start_x_, INVALID_STDINTERFACE_NLP,
                     "No initial point for the variables provided.");
    ASSERT_EXCEPTION(eval_f_, INVALID_STDINTERFACE_NLP,
                     "No callback function for evaluating the value of objective function provided.");
    ASSERT_EXCEPTION(eval_g_, INVALID_STDINTERFACE_NLP,
                     "No callback function for evaluating the values of constraints provided.");
    ASSERT_EXCEPTION(eval_grad_f_, INVALID_STDINTERFACE_NLP,
                     "No callback function for evaluating the gradient of objective function provided.");
    ASSERT_EXCEPTION(eval_jac_g_, INVALID_STDINTERFACE_NLP,
                     "No callback function for evaluating the Jacobian of the constraints provided.");
    ASSERT_EXCEPTION(eval_h_, INVALID_STDINTERFACE_NLP,
                     "No callback function for evaluating the Hessian of the constraints provided.");
  }

  StdInterfaceTNLP::~StdInterfaceTNLP()
  {
    delete [] non_const_x_;
  }

  bool StdInterfaceTNLP::get_nlp_info(Index& n, Index& m, Index& nnz_jac_g,
                                      Index& nnz_h_lag)
  {
    n = n_var_; // # of variables (variable types have been asserted in the constructor
    m = n_con_; // # of constraints
    nnz_jac_g = nele_jac_; // # of non-zeros in the jacobian
    nnz_h_lag = nele_hess_; // # of non-zeros in the hessian

    return true;
  }

  bool StdInterfaceTNLP::get_bounds_info(Index n, Number* x_l, Number* x_u,
                                         Index m, Number* d_l, Number* d_u)
  {
    DBG_ASSERT(n == n_var_);
    DBG_ASSERT(m == n_con_);

    for (Index i=0; i<n; i++) {
      x_l[i] = x_L_[i];
      x_u[i] = x_U_[i];
    }

    for (Index i=0; i<m; i++) {
      d_l[i] = g_L_[i];
      d_u[i] = g_U_[i];
    }

    return true;
  }

  bool StdInterfaceTNLP::get_starting_point(Index n, bool init_x,
      Number* x, bool init_z,
      Number* z_L, Number* z_U,
      Index m, bool init_lambda,
      Number* lambda)
  {
    bool retval=true;

    DBG_ASSERT(n == n_var_);
    DBG_ASSERT(m == n_con_);

    if (init_x) {
      for (Index i=0; i<n; i++) {
        x[i] = start_x_[i];
      }
    }

    if (init_z) {
      if (start_z_L_==NULL) {
        retval = false;
      }
      else {
        for (Index i=0; i<n; i++) {
          z_L[i] = start_z_L_[i];
        }
      }
      if (start_z_U_==NULL) {
        retval = false;
      }
      else {
        for (Index i=0; i<n; i++) {
          z_U[i] = start_z_U_[i];
        }
      }
    }

    if (init_lambda) {
      if(start_lam_==NULL) {
        retval = false;
      }
      else {
        for (Index i=0; i<m; i++) {
          lambda[i] = start_lam_[i];
        }
      }
    }

    return retval;
  }

  bool StdInterfaceTNLP::eval_f(Index n, const Number* x, bool new_x,
                                Number& obj_value)
  {
    DBG_ASSERT(n==n_var_);

    apply_new_x(new_x, n, x);

    Bool retval = (*eval_f_)(n, non_const_x_, (Bool)new_x,
                             &obj_value, user_data_);
    return (retval!=0);
  }

  bool StdInterfaceTNLP::eval_grad_f(Index n, const Number* x, bool new_x,
                                     Number* grad_f)
  {
    DBG_ASSERT(n==n_var_);

    apply_new_x(new_x, n, x);

    Bool retval = (*eval_grad_f_)(n, non_const_x_, (Bool)new_x, grad_f,
                                  user_data_);
    return (retval!=0);
  }

  bool StdInterfaceTNLP::eval_g(Index n, const Number* x, bool new_x,
                                Index m, Number* g)
  {
    DBG_ASSERT(n==n_var_);
    DBG_ASSERT(m==n_con_);

    apply_new_x(new_x, n, x);

    Bool retval = (*eval_g_)(n, non_const_x_, (Bool)new_x, m, g, user_data_);

    return (retval!=0);
  }

  bool StdInterfaceTNLP::eval_jac_g(Index n, const Number* x, bool new_x,
                                    Index nele_jac, Index* iRow, Index *jCol,
                                    Number* values)
  {
    DBG_ASSERT(n==n_var_);
    DBG_ASSERT(nele_jac==nele_jac_);

    Bool retval=1;

    if ( (iRow && jCol && !values) || (!iRow && !jCol && values) ) {
      apply_new_x(new_x, n, x);
      retval = (*eval_jac_g_)(n, non_const_x_, (Bool)new_x, nele_jac,
                              iRow, jCol, values, user_data_);
    }
    else {
      DBG_ASSERT(false && "Invalid combination of iRow, jCol, and values pointers");
    }
    return (retval!=0);
  }

  bool StdInterfaceTNLP::eval_h(Index n, const Number* x, bool new_x,
                                Number obj_factor, Index m,
                                const Number* lambda, bool new_lambda,
                                Index nele_hess, Index* iRow, Index* jCol,
                                Number* values)
  {
    DBG_ASSERT(n==n_var_);
    DBG_ASSERT(m==n_con_);
    DBG_ASSERT(nele_hess==nele_hess_);

    Bool retval=1;

    if ( (iRow && jCol && !values) || (!iRow && !jCol && values) ) {
      apply_new_x(new_x, n, x);
      Number* non_const_lambda = new Number[m];
      if (lambda) {
        for (Index i=0; i<m; i++) {
          non_const_lambda[i] = lambda[i];
        }
      }

      retval = (*eval_h_)(n, non_const_x_, (Bool)new_x, obj_factor, m,
                          non_const_lambda, (Bool)new_lambda, nele_hess,
                          iRow, jCol, values, user_data_);
      delete [] non_const_lambda;
    }
    else {
      DBG_ASSERT(false && "Invalid combination of iRow, jCol, and values pointers");
    }
    return (retval!=0);
  }

  void StdInterfaceTNLP::apply_new_x(bool new_x, Index n, const Number* x)
  {
    if (new_x) {
      //copy the data to the non_const_x_
      if (!non_const_x_) {
        non_const_x_ = new Number[n];
      }

      DBG_ASSERT(x && "x is NULL");
      for (Index i=0; i<n; i++) {
        non_const_x_[i] = x[i];
      }
    }
  }

} // namespace Ipopt


