// Copyright (C) 2004, International Business Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-08-13

#ifndef __IPIPOPTNLP_HPP__
#define __IPIPOPTNLP_HPP__

#include "IpUtils.hpp"
#include "IpNLP.hpp"
#include "IpJournalist.hpp"
#include "IpReferenced.hpp"
#include "IpException.hpp"

namespace Ipopt
{

  /** This is the abstract base class for classes that map
   *  the traditional NLP into
   *  something that is more useful by Ipopt.
   *  This class takes care of storing the
   *  calculated model results, handles cacheing,
   *  and (some day) takes care of addition of slacks.
   */
  class IpoptNLP : public ReferencedObject
  {
  public:
    /**@name Constructors/Destructors */
    //@{
    IpoptNLP()
    {}
    ;

    /** Default destructor */
    virtual ~IpoptNLP()
    {}
    ;
    //@}

    /**@name Possible Exceptions */
    //@{
    /** thrown if there is any error evaluating values from the nlp */
    DECLARE_STD_EXCEPTION(Eval_Error);
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
                                     ) = 0;

    /** Accessor methods for model data */
    //@{
    /** Objective value */
    virtual Number f(const Vector& x) = 0;

    /** Gradient of the objective */
    virtual SmartPtr<const Vector> grad_f(const Vector& x) = 0;

    /** Equality constraint residual */
    virtual SmartPtr<const Vector> c(const Vector& x) = 0;

    /** Jacobian Matrix for equality constraints
     *  (current iteration) */
    virtual SmartPtr<const Matrix> jac_c(const Vector& x) = 0;

    /** Inequality constraint residual (reformulated
     *  as equalities with slacks */
    virtual SmartPtr<const Vector> d(const Vector& x) = 0;

    /** Jacobian Matrix for inequality constraints
     *  (current iteration) */
    virtual SmartPtr<const Matrix> jac_d(const Vector& x) = 0;

    /** Hessian of the lagrangian
     *  (current iteration) */
    virtual SmartPtr<const SymMatrix> h(const Vector& x,
                                        Number obj_factor,
                                        const Vector& yc,
                                        const Vector& yd
                                       ) = 0;

    /** Lower bounds on x */
    virtual SmartPtr<const Vector> x_L() = 0;

    /** Permutation matrix (x_L_ -> x) */
    virtual SmartPtr<const Matrix> Px_L() = 0;

    /** Upper bounds on x */
    virtual SmartPtr<const Vector> x_U() = 0;

    /** Permutation matrix (x_U_ -> x */
    virtual SmartPtr<const Matrix> Px_U() = 0;

    /** Lower bounds on d */
    virtual SmartPtr<const Vector> d_L() = 0;

    /** Permutation matrix (d_L_ -> d) */
    virtual SmartPtr<const Matrix> Pd_L() = 0;

    /** Upper bounds on d */
    virtual SmartPtr<const Vector> d_U() = 0;

    /** Permutation matrix (d_U_ -> d */
    virtual SmartPtr<const Matrix> Pd_U() = 0;
    //@}

    /** Accessor method for vector/matrix spaces pointers. */
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
                           SmartPtr<const SymMatrixSpace>& Hess_lagrangian_space) = 0;

    /** Method for adapting the variable bounds.  This is called if
     *  slacks are becoming too small */
    virtual void AdjustVariableBounds(const Vector& new_x_L,
                                      const Vector& new_x_U,
                                      const Vector& new_d_L,
                                      const Vector& new_d_U)=0;

    /** @name Counters for the number of function evaluations. */
    //@{
    virtual Index f_evals() const = 0;
    virtual Index grad_f_evals() const = 0;
    virtual Index c_evals() const = 0;
    virtual Index jac_c_evals() const = 0;
    virtual Index d_evals() const = 0;
    virtual Index jac_d_evals() const = 0;
    virtual Index h_evals() const = 0;
    //@}

  private:

    /**@name Default Compiler Generated Methods
     * (Hidden to avoid implicit creation/calling).
     * These methods are not implemented and 
     * we do not want the compiler to implement
     * them for us, so we declare them private
     * and do not define them. This ensures that
     * they will not be implicitly created/called. */
    //@{

    /** Copy Constructor */
    IpoptNLP(const IpoptNLP&);

    /** Overloaded Equals Operator */
    void operator=(const IpoptNLP&);
    //@}
  };

} // namespace Ipopt

#endif