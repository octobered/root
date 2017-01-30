// @(#)root/mathmore:$Id$
// Author: L. Moneta Wed Sep  6 09:52:26 2006

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2006  LCG ROOT Math Team, CERN/PH-SFT                *
 *                                                                    *
 *                                                                    *
 **********************************************************************/

// Header file for class WrappedTFunction

#ifndef ROOT_Math_WrappedMultiTF1
#define ROOT_Math_WrappedMultiTF1


#ifndef ROOT_Math_IParamFunction
#include "Math/IParamFunction.h"
#endif

#ifndef ROOT_TF1
#include "TF1.h"
#endif

#include "TClass.h"   // needed to copy the TF1 pointer

namespace ROOT {

   namespace Math {


/**
   Class to Wrap a ROOT Function class (like TF1)  in a IParamMultiFunction interface
   of multi-dimensions to be used in the ROOT::Math numerical algorithm.
   This wrapper class does not own the TF1 pointer, so it assumes it exists during the wrapper lifetime.
   The class copy the TF1 pointer only when it owns it.

   The class from ROOT version 6.03 does not contain anymore a copy of the parameters. The parameters are
   stored in the TF1 class.

   @ingroup CppFunctions
*/

//LM note: are there any issues when cloning the class for the parameters that are not copied anymore ??

template<class T>
class WrappedMultiTF1Templ: virtual public ROOT::Math::IParametricGradFunctionMultiDimTempl<T>{

public:

   typedef  ROOT::Math::IParametricGradFunctionMultiDimTempl<T>  BaseParamFunc;
   typedef  typename ROOT::Math::IParametricFunctionMultiDimTempl<T>::BaseFunc  BaseFunc;

   /**
      constructor from a function pointer to a TF1
      If dim = 0 dimension is taken from TF1::GetNdim().
      IN case of multi-dimensional function created using directly TF1 object the dimension
      returned by TF1::GetNdim is always 1. The user must then pass the correct value of dim
   */
   WrappedMultiTF1Templ(TF1 & f, unsigned int dim = 0 );

   /**
      Destructor (no operations). Function pointer is not owned
   */
   ~WrappedMultiTF1Templ () { if (fOwnFunc && fFunc) delete fFunc; }

   /**
      Copy constructor
   */
   WrappedMultiTF1Templ(const WrappedMultiTF1Templ<T> & rhs);

   /**
      Assignment operator
   */
   WrappedMultiTF1Templ & operator = (const WrappedMultiTF1Templ<T> & rhs);

    /** @name interface inherited from IParamFunction */

   /**
       Clone the wrapper but not the original function
   */
   IMultiGenFunctionTempl<T> * Clone() const {
      return new WrappedMultiTF1Templ<T>(*this);
   }

   /**
        Retrieve the dimension of the function
    */
    unsigned int NDim() const {
        return fDim;
    }

   /// get the parameter values (return values from TF1)
   const double * Parameters() const {
  //return  (fParams.size() > 0) ? &fParams.front() : 0;
      return  fFunc->GetParameters();
   }

   /// set parameter values (only the cached one in this class,leave unchanges those of TF1)
   void SetParameters(const double * p) {
      //std::copy(p,p+fParams.size(),fParams.begin());
      fFunc->SetParameters(p);
   }

   /// return number of parameters
   unsigned int NPar() const {
      // return fParams.size();
      return fFunc->GetNpar();
   }

   // evaluate the derivative of the function with respect to the parameters
    void  ParameterGradient(const double * x, const double * par, double * grad ) const;

   /// precision value used for calculating the derivative step-size
   /// h = eps * |x|. The default is 0.001, give a smaller in case function changes rapidly
   void SetDerivPrecision(double eps);

   /// get precision value used for calculating the derivative step-size
   double GetDerivPrecision();

   /// method to retrieve the internal function pointer
   const TF1 * GetFunction() const { return fFunc; }

   /// method to set a new function pointer and copy it inside.
   /// By calling this method the class manages now the passed TF1 pointer
   void SetAndCopyFunction(const TF1 * f = 0);

private:
   /// evaluate function passing coordinates x and vector of parameters
   T DoEvalPar (const T * x, const double * p ) const {
      return fFunc->EvalPar(x,p);
   }

   /// evaluate function using the cached parameter values (of TF1)
   /// re-implement for better efficiency
   T DoEvalVec (const T * x) const {
      return fFunc->EvalPar(x, 0 );
   }

   /// evaluate function passing coordinates x and vector of parameters
//    double DoEvalPar (const double * x, const double * p ) const {
//       if (fFunc->GetMethodCall() )  fFunc->InitArgs(x,p);  // needed for interpreted functions
//       return fFunc->EvalPar(x,p);
//    }

   /// evaluate function using the cached parameter values (of TF1)
   /// re-implement for better efficiency
   T DoEval (const T* x) const {
      // no need to call InitArg for interpreted functions (done in ctor)

      //const double * p = (fParams.size() > 0) ? &fParams.front() : 0;
      return fFunc->EvalPar(x, 0 );
   }

   /// evaluate the partial derivative with respect to the parameter
   double DoParameterDerivative(const double * x, const double * p, unsigned int ipar) const;

   bool fLinear;                 // flag for linear functions
   bool fPolynomial;             // flag for polynomial functions
   bool fOwnFunc;                 // flag to indicate we own the TF1 function pointer
   TF1 * fFunc;                   // pointer to ROOT function
   unsigned int fDim;             // cached value of dimension
   //std::vector<double> fParams;   // cached vector with parameter values

   double fEps;          // epsilon used in derivative calculation h ~ eps |p|
};

// impelmentations for WrappedMultiTF1Templ<T>
template<class T>
WrappedMultiTF1Templ<T>::WrappedMultiTF1Templ (TF1 & f, unsigned int dim  )  :
   fLinear(false),
   fPolynomial(false),
   fOwnFunc(false),
   fFunc(&f),
   fDim(dim),
   fEps(0.001)
   //fParams(f.GetParameters(),f.GetParameters()+f.GetNpar())
{
   // constructor of WrappedMultiTF1Templ<T>
   // pass a dimension if dimension specified in TF1 does not correspond to real dimension
   // for example in case of multi-dimensional TF1 objects defined as TF1 (i.e. for functions with dims > 3 )
   if (fDim == 0) fDim = fFunc->GetNdim();

   // check that in case function is linear the linear terms are not zero
   // function is linear when is a TFormula created with "++"
   // hyperplane are not yet existing in TFormula
   if (fFunc->IsLinear() ) {
      int ip = 0;
      fLinear = true;
      while (fLinear && ip < fFunc->GetNpar() )  {
         fLinear &= (fFunc->GetLinearPart(ip) != 0) ;
         ip++;
      }
   }
   // distinguish case of polynomial functions and linear functions
   if (fDim == 1 && fFunc->GetNumber() >= 300 && fFunc->GetNumber() < 310) {
      fLinear = true;
      fPolynomial = true;
   }
}

template<class T>
WrappedMultiTF1Templ<T>::WrappedMultiTF1Templ(const WrappedMultiTF1Templ<T> & rhs) :
   BaseFunc(),
   BaseParamFunc(),
   fLinear(rhs.fLinear),
   fPolynomial(rhs.fPolynomial),
   fOwnFunc(rhs.fOwnFunc),
   fFunc(rhs.fFunc),
   fDim(rhs.fDim)
   //fParams(rhs.fParams)
{
   // copy constructor
   if (fOwnFunc) SetAndCopyFunction(rhs.fFunc);
}

template<class T>
WrappedMultiTF1Templ<T> & WrappedMultiTF1Templ<T>::operator= (const WrappedMultiTF1Templ<T> & rhs) {
   // Assignment operator
   if (this == &rhs) return *this;  // time saving self-test
   fLinear = rhs.fLinear;
   fPolynomial = rhs.fPolynomial;
   fOwnFunc = rhs.fOwnFunc;
   fDim = rhs.fDim;
   //fParams = rhs.fParams;
   return *this;
}

template<class T>
void  WrappedMultiTF1Templ<T>::ParameterGradient(const double * x, const double * par, double * grad ) const {
   // evaluate the gradient of the function with respect to the parameters
   //IMPORTANT NOTE: TF1::GradientPar returns 0 for fixed parameters to avoid computing useless derivatives
   //  BUT the TLinearFitter wants to have the derivatives also for fixed parameters.
   //  so in case of fLinear (or fPolynomial) a non-zero value will be returned for fixed parameters

   if (!fLinear) {
      // need to set parameter values
      fFunc->SetParameters( par );
      // no need to call InitArgs (it is called in TF1::GradientPar)
      fFunc->GradientPar(x,grad,fEps);
   }
   else {  // case of linear functions
      unsigned int np = NPar();
      for (unsigned int i = 0; i < np; ++i)
         grad[i] = DoParameterDerivative(x, par, i);
   }
}

template<class T>
double WrappedMultiTF1Templ<T>::DoParameterDerivative(const double * x, const double * p, unsigned int ipar ) const {
   // evaluate the derivative of the function with respect to parameter ipar
   // see note above concerning the fixed parameters
   if (! fLinear ) {
      fFunc->SetParameters( p );
      return fFunc->GradientPar(ipar, x,fEps);
   }
   if (fPolynomial) {
      // case of polynomial function (no parameter dependency)  (case for dim = 1)
      assert (fDim == 1);
      if (ipar == 0) return 1.0;
      return std::pow(x[0], static_cast<int>(ipar) );
   }
   else {
      // case of general linear function (built in TFormula with ++ )
      const TFormula * df = dynamic_cast<const TFormula*>( fFunc->GetLinearPart(ipar) );
      assert(df != 0);
      return (const_cast<TFormula*> ( df) )->EvalPar( x ) ; // derivatives should not depend on parameters since
                                                            // function  is linear
   }
}

template<class T>
void WrappedMultiTF1Templ<T>::SetDerivPrecision(double eps) { fEps = eps; }

template<class T>
double WrappedMultiTF1Templ<T>::GetDerivPrecision( ) { return fEps; }

template<class T>
void WrappedMultiTF1Templ<T>::SetAndCopyFunction(const TF1 * f) {
   const TF1 * funcToCopy = (f) ? f : fFunc;
   TF1 * fnew = (TF1*) funcToCopy->IsA()->New();
   funcToCopy->Copy(*fnew);
   fFunc = fnew;
   fOwnFunc = true;
}

using WrappedMultiTF1 = WrappedMultiTF1Templ<double>;

   } // end namespace Math

} // end namespace ROOT


#endif /* ROOT_Fit_WrappedMultiTF1 */
