// Copyright (C) 2004, International Business Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-08-13

#ifndef __IPRESTOIPOPTNLP_HPP__
#define __IPRESTOIPOPTNLP_HPP__

#include "IpIpoptNLP.hpp"
#include "IpIpoptData.hpp"
#include "IpIpoptCalculatedQuantities.hpp"
#include "IpCompoundMatrix.hpp"
#include "IpCompoundSymMatrix.hpp"
#include "IpCompoundVector.hpp"
#include "IpIdentityMatrix.hpp"
#include "IpDiagMatrix.hpp"
#include "IpZeroMatrix.hpp"

namespace Ipopt
{

  /** This class maps the traditional NLP into
   *  something that is more useful by Ipopt.
   *  This class takes care of storing the
   *  calculated model results, handles cacheing,
   *  and (some day) takes care of addition of slacks.
   */
  class RestoIpoptNLP : public IpoptNLP
  {
  public:
    /**@name Constructors/Destructors */
    //@{
    RestoIpoptNLP(IpoptNLP& orig_ip_nlp,
                  IpoptData& orig_ip_data,
                  IpoptCalculatedQuantities& orig_ip_cq,
                  IpoptData& curr_ip_data);

    /** Default destructor */
    ~RestoIpoptNLP();
    //@}

    /** Initialize (create) structures for
     *  the iteration data */
    virtual bool InitializeStructures(SmartPtr<Vector>& x,
                                      bool init_x,
                                      SmartPtr<Vector>& y_c,
                                      bool init_y_c,
                                      SmartPtr<Vector>& y_d,
                                      bool init_y_d,
                                      SmartPtr<Vector>& z_L,
                                      bool init_z_L,
                                      SmartPtr<Vector>& z_U,
                                      bool init_z_U,
                                      SmartPtr<Vector>& v_L,
                                      bool init_v_L,
                                      SmartPtr<Vector>& v_U,
                                      bool init_v_U
                                     );

    /** Accessor methods for model data */
    //@{
    /** Objective value */
    virtual Number f(const Vector& x);

    /** Gradient of the objective */
    virtual SmartPtr<const Vector> grad_f(const Vector& x);

    /** Equality constraint residual */
    virtual SmartPtr<const Vector> c(const Vector& x);

    /** Jacobian Matrix for equality constraints
     *  (current iteration) */
    virtual SmartPtr<const Matrix> jac_c(const Vector& x);

    /** Inequality constraint residual (reformulated
     *  as equalities with slacks */
    virtual SmartPtr<const Vector> d(const Vector& x);

    /** Jacobian Matrix for inequality constraints
     *  (current iteration) */
    virtual SmartPtr<const Matrix> jac_d(const Vector& x);

    /** Hessian of the lagrangian
     *  (current iteration) */
    virtual SmartPtr<const SymMatrix> h(const Vector& x,
                                        Number obj_factor,
                                        const Vector& yc,
                                        const Vector& yd
                                       );

    /** Lower bounds on x */
    virtual SmartPtr<const Vector> x_L()
    {
      return GetRawPtr(x_L_);
    }

    /** Permutation matrix (x_L_ -> x) */
    virtual SmartPtr<const Matrix> Px_L()
    {
      return GetRawPtr(Px_L_);
    }

    /** Upper bounds on x */
    virtual SmartPtr<const Vector> x_U()
    {
      return GetRawPtr(x_U_);
    }

    /** Permutation matrix (x_U_ -> x */
    virtual SmartPtr<const Matrix> Px_U()
    {
      return GetRawPtr(Px_U_);
    }

    /** Lower bounds on d */
    virtual SmartPtr<const Vector> d_L()
    {
      return GetRawPtr(d_L_);
    }

    /** Permutation matrix (d_L_ -> d) */
    virtual SmartPtr<const Matrix> Pd_L()
    {
      return GetRawPtr(Pd_L_);
    }

    /** Upper bounds on d */
    virtual SmartPtr<const Vector> d_U()
    {
      return GetRawPtr(d_U_);
    }

    /** Permutation matrix (d_U_ -> d */
    virtual SmartPtr<const Matrix> Pd_U()
    {
      return GetRawPtr(Pd_U_);
    }
    //@}

    /** Accessor method for vector/matrix spaces pointers */
    virtual void GetSpaces(SmartPtr<const VectorSpace>& x_space,
                           SmartPtr<const VectorSpace>& c_space,
                           SmartPtr<const VectorSpace>& d_space,
                           SmartPtr<const VectorSpace>& x_l_space,
                           SmartPtr<const MatrixSpace>& px_l_space,
                           SmartPtr<const VectorSpace>& x_u_space,
                           SmartPtr<const MatrixSpace>& px_u_space,
                           SmartPtr<const VectorSpace>& d_l_space,
                           SmartPtr<const MatrixSpace>& pd_l_space,
                           SmartPtr<const VectorSpace>& d_u_space,
                           SmartPtr<const MatrixSpace>& pd_u_space,
                           SmartPtr<const MatrixSpace>& Jac_c_space,
                           SmartPtr<const MatrixSpace>& Jac_d_space,
                           SmartPtr<const SymMatrixSpace>& Hess_lagrangian_space);
    /** Method for adapting the variable bounds.  This is called if
     *  slacks are becoming too small */
    virtual void AdjustVariableBounds(const Vector& new_x_L,
                                      const Vector& new_x_U,
                                      const Vector& new_d_L,
                                      const Vector& new_d_U);

    /** @name Accessor method for the information of the original NLP.
     *  These methods are not overloaded from IpoptNLP */
    //@{
    IpoptNLP& OrigIpNLP() const
    {
      return *orig_ip_nlp_;
    }
    IpoptData& OrigIpData() const
    {
      return *orig_ip_data_;
    }
    IpoptCalculatedQuantities& OrigIpCq() const
    {
      return *orig_ip_cq_;
    }
    //@}

    /** Accessor Method for obtaining the Rho penalization factor for
     *  the ell_1 norm */
    Number Rho() const
    {
      return rho_;
    }

    /** @name Counters for the number of function evaluations. */
    //@{
    virtual Index f_evals() const
    {
      return f_evals_;
    }
    virtual Index grad_f_evals() const
    {
      return grad_f_evals_;
    }
    virtual Index c_evals() const
    {
      return c_evals_;
    }
    virtual Index jac_c_evals() const
    {
      return jac_c_evals_;
    }
    virtual Index d_evals() const
    {
      return d_evals_;
    }
    virtual Index jac_d_evals() const
    {
      return jac_d_evals_;
    }
    virtual Index h_evals() const
    {
      return h_evals_;
    }
    //@}

  private:
    /** @name Pointers for the original NLP information. */
    //@{
    /** Pointer to the original IpoptNLP */
    SmartPtr<IpoptNLP> orig_ip_nlp_;

    /** Pointer to the original IpoptData */
    SmartPtr<IpoptData> orig_ip_data_;

    /** Pointer to the original IpoptCalculatedQuantities */
    SmartPtr<IpoptCalculatedQuantities> orig_ip_cq_;
    //@}

    /** Pointer to the current IpoptData (to get the value for mu) */
    SmartPtr<IpoptData> ip_data_;

    /** Necessary Vector/Matrix spaces */
    //@{
    SmartPtr<CompoundVectorSpace> x_space_;

    SmartPtr<const VectorSpace> c_space_;

    SmartPtr<const VectorSpace> d_space_;

    SmartPtr<CompoundVectorSpace> x_l_space_;

    SmartPtr<CompoundMatrixSpace> px_l_space_;

    SmartPtr<const VectorSpace> x_u_space_;

    SmartPtr<CompoundMatrixSpace> px_u_space_;

    SmartPtr<const VectorSpace> d_l_space_;

    SmartPtr<const MatrixSpace> pd_l_space_;

    SmartPtr<const VectorSpace> d_u_space_;

    SmartPtr<const MatrixSpace> pd_u_space_;

    SmartPtr<CompoundMatrixSpace> jac_c_space_;

    SmartPtr<CompoundMatrixSpace> jac_d_space_;

    SmartPtr<CompoundSymMatrixSpace> h_space_;
    //@}

    /**@name Storage for Model Quantities */
    //@{
    /** Lower bounds on x */
    SmartPtr<CompoundVector> x_L_;

    /** Permutation matrix (x_L_ -> x) */
    SmartPtr<CompoundMatrix> Px_L_;

    /** Upper bounds on x */
    SmartPtr<const Vector> x_U_;

    /** Permutation matrix (x_U_ -> x) */
    SmartPtr<CompoundMatrix> Px_U_;

    /** Lower bounds on d */
    SmartPtr<const Vector> d_L_;

    /** Permutation matrix (d_L_ -> d) */
    SmartPtr<const Matrix> Pd_L_;

    /** Upper bounds on d */
    SmartPtr<const Vector> d_U_;

    /** Permutation matrix (d_U_ -> d */
    SmartPtr<const Matrix> Pd_U_;
    //@}

    /** @name Values particular to the restoration phase problem statement */
    //@{
    /** Penalty parameter for the \$l_1\$ norm */
    Number rho_;
    /** scaling factor for eta calculation */
    Number eta_factor_;
    /** exponent for mu in eta calculation */
    Number eta_mu_exponent_;
    // TODO in the following we should use pointers to CONST values
    /** Scaling factors for the \$x\$ part of the regularization term */
    SmartPtr<Vector> dr_x_;
    SmartPtr<DiagMatrix> DR_x_; //TODO We can get rid of one of the dr DR
    /** \$x\$ part of the reference point in the regularization term */
    SmartPtr<Vector> x_ref_;
    //@}

    /** Method to calculate eta, the factor for the restoration term */
    Number Eta() const;

    /**@name Default Compiler Generated Methods
     * (Hidden to avoid implicit creation/calling).
     * These methods are not implemented and 
     * we do not want the compiler to implement
     * them for us, so we declare them private
     * and do not define them. This ensures that
     * they will not be implicitly created/called. */
    //@{
    /** Default Constructor */
    RestoIpoptNLP();

    /** Copy Constructor */
    RestoIpoptNLP(const RestoIpoptNLP&);

    /** Overloaded Equals Operator */
    void operator=(const RestoIpoptNLP&);
    //@}

    /** @name Counters for the function evaluations */
    //@{
    Index f_evals_;
    Index grad_f_evals_;
    Index c_evals_;
    Index jac_c_evals_;
    Index d_evals_;
    Index jac_d_evals_;
    Index h_evals_;
    //@}
  };

} // namespace Ipopt

#endif