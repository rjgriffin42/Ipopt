// Copyright (C) 2004, International Business Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-08-13

#ifndef __IPSUMSYMMATRIX_HPP__
#define __IPSUMSYMMATRIX_HPP__

#include "IpUtils.hpp"
#include "IpSymMatrix.hpp"

namespace Ipopt
{

  /* forward declarations */
  class SumSymMatrixSpace;

  /** Class for Matrices which are sum of symmetric matrices.
   *  For each term in the we store the matrix and a factor.
   */
  class SumSymMatrix : public SymMatrix
  {
  public:

    /**@name Constructors / Destructors */
    //@{

    /** Constructor, initializing with dimensions of the matrix and
     *  the number of terms in the sum.
     */
    SumSymMatrix(const SumSymMatrixSpace* owner_space);

    /** Destructor */
    ~SumSymMatrix();
    //@}

    /** Method for setting term iterm for the sum.  Note that counting
     *  of terms starts at 0. */
    void SetTerm(Index iterm, Number factor, const SymMatrix& matrix);

    /** Method for getting term iterm for the sum.  Note that counting
     *  of terms starts at 0. */
    void GetTerm(Index iterm, Number& factor, SmartPtr<const SymMatrix>& matrix) const;

    /** Return the number of terms */
    Index NTerms() const;

  protected:
    /**@name Methods overloaded from matrix */
    //@{
    virtual void MultVectorImpl(Number alpha, const Vector& x,
                                Number beta, Vector& y) const;

    virtual void PrintImpl(FILE* fp, std::string name, Index indent, std::string prefix) const;
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
    /** Default Constructor */
    SumSymMatrix();

    /** Copy Constructor */
    SumSymMatrix(const SumSymMatrix&);

    /** Overloaded Equals Operator */
    void operator=(const SumSymMatrix&);
    //@}

    /** std::vector storing the factors for each term. */
    std::vector<Number> factors_;

    /** std::vector storing the matrices for each term. */
    std::vector<SmartPtr<const SymMatrix> > matrices_;

    /** Copy of the owner_space as a SumSymMatrixSpace */
    const SumSymMatrixSpace* owner_space_;
  };

  /** Class for matrix space for SumSymMatrix */
  class SumSymMatrixSpace : public SymMatrixSpace
  {
  public:
    /** @name Constructors / Destructors */
    //@{
    /** Constructor, given the dimension of the matrix and the number
     *  of terms in the sum. */
    SumSymMatrixSpace(Index ndim, Index nterms)
        :
        SymMatrixSpace(ndim),
        nterms_(nterms)
    {}

    /** Destructor */
    ~SumSymMatrixSpace()
    {}
    //@}

    /** @name Accessor functions */
    //@{
    /** Number of terms in the sum. */
    Index NTerms() const
    {
      return nterms_;
    }
    //@}

    /** Method for creating a new matrix of this specific type. */
    SumSymMatrix* MakeNewSumSymMatrix() const;

    /** Overloaded MakeNew method for the SymMatrixSpace base class.
     */
    virtual SymMatrix* MakeNewSymMatrix() const;

  private:
    Index nterms_;
  };

} // namespace Ipopt
#endif