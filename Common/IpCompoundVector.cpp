// Copyright (C) 2004, International BusinDess Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id$
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-08-13

#include "IpCompoundVector.hpp"

#ifdef OLD_C_HEADERS
# include <math.h>
#else
# include <cmath>
#endif

#include <limits>

namespace Ipopt
{

  static const Index dbg_verbosity = 0;

  CompoundVector::CompoundVector(const CompoundVectorSpace* owner_space, bool create_new)
      :
      Vector(owner_space),
      owner_space_(owner_space),
      comps_(owner_space->NCompSpaces()),
      const_comps_(owner_space->NCompSpaces()),
      vectors_valid_(false)
  {
    Index dim_check = 0;
    for (Index i=0; i<NComps(); i++) {
      SmartPtr<const VectorSpace> space = owner_space_->GetCompSpace(i);
      DBG_ASSERT(IsValid(space));
      dim_check += space->Dim();

      if (create_new) {
        comps_[i] = space->MakeNew();
      }
    }

    DBG_ASSERT(dim_check == Dim());

    if (create_new) {
      vectors_valid_ = VectorsValid();
    }
  }

  CompoundVector::~CompoundVector()
  {
    // ToDo: Do we need an empty here?
  }

  void CompoundVector::SetComp(Index icomp, const Vector& vec)
  {
    DBG_ASSERT(icomp<NComps());
    comps_[icomp] = NULL;
    const_comps_[icomp] = &vec;

    vectors_valid_ = VectorsValid();
    ObjectChanged();
  }

  void CompoundVector::SetCompNonConst(Index icomp, Vector& vec)
  {
    DBG_ASSERT(icomp < NComps());
    comps_[icomp] = &vec;
    const_comps_[icomp] = NULL;

    vectors_valid_ = VectorsValid();
    ObjectChanged();
  }

  void CompoundVector::CopyImpl(const Vector& x)
  {
    DBG_START_METH("CompoundVector::CopyImpl(const Vector& x)", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    const CompoundVector* comp_x = dynamic_cast<const CompoundVector*>(&x);
    DBG_ASSERT(comp_x);
    DBG_ASSERT(NComps() == comp_x->NComps());
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->Copy(*comp_x->GetComp(i));
    }
  }

  void CompoundVector::ScalImpl(Number alpha)
  {
    DBG_START_METH("CompoundVector::ScalImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    for(Index i=0; i<NComps(); i++) {
      DBG_ASSERT(Comp(i));
      Comp(i)->Scal(alpha);
    }
  }

  void CompoundVector::AxpyImpl(Number alpha, const Vector &x)
  {
    DBG_START_METH("CompoundVector::AxpyImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    const CompoundVector* comp_x = dynamic_cast<const CompoundVector*>(&x);
    DBG_ASSERT(comp_x);
    DBG_ASSERT(NComps() == comp_x->NComps());
    for(Index i=0; i<NComps(); i++) {
      DBG_ASSERT(Comp(i));
      Comp(i)->Axpy(alpha, *comp_x->GetComp(i));
    }
  }

  Number CompoundVector::DotImpl(const Vector &x) const
  {
    DBG_START_METH("CompoundVector::DotImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    const CompoundVector* comp_x = dynamic_cast<const CompoundVector*>(&x);
    DBG_ASSERT(comp_x);
    DBG_ASSERT(NComps() == comp_x->NComps());
    Number dot = 0.;
    for(Index i=0; i<NComps(); i++) {
      DBG_ASSERT(ConstComp(i));
      dot += ConstComp(i)->Dot(*comp_x->GetComp(i));
    }
    return dot;
  }

  Number CompoundVector::Nrm2Impl() const
  {
    DBG_START_METH("CompoundVector::Nrm2Impl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    Number sum=0.;
    for(Index i=0; i<NComps(); i++) {
      Number nrm2 = ConstComp(i)->Nrm2();
      sum += nrm2*nrm2;
    }
    return sqrt(sum);
  }

  Number CompoundVector::AsumImpl() const
  {
    DBG_START_METH("CompoundVector::AsumImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    Number sum=0.;
    for(Index i=0; i<NComps(); i++) {
      sum += ConstComp(i)->Asum();
    }
    return sum;
  }

  Number CompoundVector::AmaxImpl() const
  {
    DBG_START_METH("CompoundVector::AmaxImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    Number max=0.;
    for(Index i=0; i<NComps(); i++) {
      max = Ipopt::Max(max, ConstComp(i)->Amax());
    }
    return max;
  }

  void CompoundVector::SetImpl(Number value)
  {
    DBG_START_METH("CompoundVector::SetImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->Set(value);
    }
  }

  void CompoundVector::ElementWiseDivideImpl(const Vector& x)
  {
    DBG_START_METH("CompoundVector::ElementWiseDivideImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    const CompoundVector* comp_x = dynamic_cast<const CompoundVector*>(&x);
    DBG_ASSERT(comp_x);
    DBG_ASSERT(NComps() == comp_x->NComps());
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->ElementWiseDivide(*comp_x->GetComp(i));
    }
  }

  void CompoundVector::ElementWiseMultiplyImpl(const Vector& x)
  {
    DBG_START_METH("CompoundVector::ElementWiseMultiplyImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    const CompoundVector* comp_x = dynamic_cast<const CompoundVector*>(&x);
    DBG_ASSERT(comp_x);
    DBG_ASSERT(NComps() == comp_x->NComps());
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->ElementWiseMultiply(*comp_x->GetComp(i));
    }
  }

  void CompoundVector::ElementWiseMaxImpl(const Vector& x)
  {
    DBG_START_METH("CompoundVector::ElementWiseMaxImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    const CompoundVector* comp_x = dynamic_cast<const CompoundVector*>(&x);
    DBG_ASSERT(comp_x);
    DBG_ASSERT(NComps() == comp_x->NComps());
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->ElementWiseMax(*comp_x->GetComp(i));
    }
  }

  void CompoundVector::ElementWiseMinImpl(const Vector& x)
  {
    DBG_START_METH("CompoundVector::ElementWiseMinImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    const CompoundVector* comp_x = dynamic_cast<const CompoundVector*>(&x);
    DBG_ASSERT(comp_x);
    DBG_ASSERT(NComps() == comp_x->NComps());
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->ElementWiseMin(*comp_x->GetComp(i));
    }
  }

  void CompoundVector::ElementWiseReciprocalImpl()
  {
    DBG_START_METH("CompoundVector::ElementWiseReciprocalImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->ElementWiseReciprocal();
    }
  }

  void CompoundVector::ElementWiseSqrtImpl()
  {
    DBG_START_METH("CompoundVector::ElementWiseSqrtImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->ElementWiseSqrt();
    }
  }

  void CompoundVector::AddScalarImpl(Number scalar)
  {
    DBG_START_METH("CompoundVector::AddScalarImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->AddScalar(scalar);
    }
  }

  Number CompoundVector::MaxImpl() const
  {
    DBG_START_METH("CompoundVector::MaxImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    DBG_ASSERT(NComps() > 0 && Dim() > 0 && "There is no Max of a zero length vector (no reasonable default can be returned)");
    Number max = -std::numeric_limits<Number>::max();
    for(Index i=0; i<NComps(); i++) {
      if (ConstComp(i)->Dim() != 0) {
        max = Ipopt::Max(max, ConstComp(i)->Max());
      }
    }
    return max;
  }

  Number CompoundVector::MinImpl() const
  {
    DBG_START_METH("CompoundVector::MinImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    DBG_ASSERT(NComps() > 0 && Dim() > 0 && "There is no Min of a zero length vector (no reasonable default can be returned)");
    Number min = std::numeric_limits<Number>::max();
    for (Index i=0; i<NComps(); i++) {
      if (ConstComp(i)->Dim() != 0) {
        min = Ipopt::Min(min, ConstComp(i)->Min());
      }
    }
    return min;
  }

  Number CompoundVector::SumImpl() const
  {
    DBG_START_METH("CompoundVector::SumImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    Number sum=0.;
    for(Index i=0; i<NComps(); i++) {
      sum += ConstComp(i)->Sum();
    }
    return sum;
  }

  Number CompoundVector::SumLogsImpl() const
  {
    DBG_START_METH("CompoundVector::SumLogsImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    Number sum=0.;
    for(Index i=0; i<NComps(); i++) {
      sum += ConstComp(i)->SumLogs();
    }
    return sum;
  }

  void CompoundVector::SgnImpl()
  {
    DBG_START_METH("CompoundVector::SgnImpl", dbg_verbosity);
    DBG_ASSERT(vectors_valid_);
    for(Index i=0; i<NComps(); i++) {
      Comp(i)->Sgn();
    }
  }

  void CompoundVector::PrintImpl(FILE* fp, std::string name, Index indent, std::string prefix) const
  {
    DBG_START_METH("CompoundVector::PrintImpl", dbg_verbosity);
    fprintf(fp, "\n");
    for (Index ind=0; ind<indent; ind++) {
      fprintf(fp, " ");
    }
    fprintf(fp, "%sCompoundVector \"%s\" with %d components:\n",
            prefix.c_str(), name.c_str(), NComps());
    for(Index i=0; i<NComps(); i++) {
      fprintf(fp, "\n");
      for (Index ind=0; ind<indent; ind++) {
        fprintf(fp, " ");
      }
      fprintf(fp, "%sComponent %d:\n", prefix.c_str(), i+1);
      if (ConstComp(i)) {
        char buffer[256];
        sprintf(buffer, "%s[%d]", name.c_str(), i);
        std::string term_name = buffer;
        ConstComp(i)->Print(fp, term_name, indent+1, prefix);
      }
      else {
        for (Index ind=0; ind<indent; ind++) {
          fprintf(fp, " ");
        }
        fprintf(fp, "%sComponent %d is not yet set!\n", prefix.c_str(), i+1);
      }
    }
  }

  bool CompoundVector::VectorsValid()
  {
    bool retVal = true;
    for (Index i=0; i<NComps(); i++) {
      // Better not have an entry in both (sanity check)
      DBG_ASSERT(IsNull(comps_[i]) || IsNull(const_comps_[i]));
      if (ConstComp(i) == NULL) {
        retVal = false;
        break;
      }
    }
    return retVal;
  }

  CompoundVectorSpace::CompoundVectorSpace(Index ncomp_spaces, Index total_dim)
      :
      VectorSpace(total_dim),
      ncomp_spaces_(ncomp_spaces),
      comp_spaces_(ncomp_spaces)
  {}

  void CompoundVectorSpace::SetCompSpace(Index icomp, const VectorSpace& vec_space)
  {
    DBG_ASSERT(icomp<ncomp_spaces_);
    DBG_ASSERT(IsNull(comp_spaces_[icomp]));
    comp_spaces_[icomp] = &vec_space;
  }

  SmartPtr<const VectorSpace> CompoundVectorSpace::GetCompSpace(Index icomp) const
  {
    DBG_ASSERT(icomp<ncomp_spaces_);
    return comp_spaces_[icomp];
  }

}