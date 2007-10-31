/* Copyright (C) 2004-2006  Martin Förg, Mathias Bachmayer, Felix Kahr
 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *  
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 *  
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 * 
 * Derived from data_interface_base Klass DataInterfaceBase. Here You'll find a bib
 * giving users the possibility of quick interaction with other objects.
 *
 * Feel free, implement Your own additional stuff derived from userfunction! 
 * The name of the game is user's function.   
 *
 * Contact:
 *   mfoerg@users.berlios.de
 *
 */

#ifndef _USERFUNCTION_H_
#define _USERFUNCTION_H_

#include "eps.h"
#include <vector>
#include "data_interface_base.h"

using namespace fmatvec;

namespace MBSim {
  class PPolynom;

  /*! Comment
   *
   * */
  class UserFunction : public DataInterfaceBase{
    private:
      double delta, sqrtdelta;

    public:
      UserFunction() : delta(epsroot()), sqrtdelta(sqrt(delta)) {}
      virtual ~UserFunction() {}
      virtual Vec operator()(double t) = 0; 
      virtual Vec diff1(double t) { 
	return (operator()(t+delta)-operator()(t-delta))/(2.0*delta);
      } 
      virtual Vec diff2(double t) {
	return (operator()(t+sqrtdelta)+operator()(t-sqrtdelta)-2*operator()(t))/(sqrtdelta*sqrtdelta);
      }; 
  };

  // Userfunction for spline interpolation or/and picewise defined polynoms see ppolynom.h

  class FuncTable : public UserFunction {
    protected:
      Mat table;
      int oldi;
    public:
      FuncTable() {oldi=0;}
      ~FuncTable() {}
      Vec operator()(double alpha) ; 
      // Vec diff1(double alpha) ; 
      // Vec diff2(double alpha) ; 
      void setTable(const Mat &tab) {table = tab;}
      void setXY(const Vec& X,const Mat& Y);
      void setFile(const string &filename); 
  };

  class FuncTablePer : public UserFunction {
    protected:
      Mat table;
    public:
      FuncTablePer() {}
      ~FuncTablePer() {}
      Vec operator()(double alpha) ; 
      // Vec diff1(double alpha) ; 
      // Vec diff2(double alpha) ; 

      void setFile(const string &filename); 
  };


  class FuncSum : public UserFunction {
    protected: 
      vector<DataInterfaceBase*> dib;
      vector<double> c;
      int outputdim;
    public:
      FuncSum() {}
      ~FuncSum();
      void addInput(DataInterfaceBase* func_,double c_,int dim);
      void addInput(DataInterfaceBase* func_,double c_=1);
      Vec operator()(double x) ;      
  };
  class FuncConst : public UserFunction {
    protected:
      Vec c;
    public:
      FuncConst(const Vec& c_);
      Vec operator()(double x) {return c;} 
  };
  class FuncLinear : public UserFunction {
    protected:
      Vec a,b;
    public:
      FuncLinear(const Vec& a_,const Vec& b_);
      Vec operator()(double x) {return a*x+b;} 
  };  
  class FuncHarmonic : public UserFunction {
    protected:
      Vec A,offset;
      double om, phi;
    public:
      FuncHarmonic(const Vec& A_,double om_, double phi_, const Vec& offset_) {A=A_; om=om_; phi=phi_; offset=offset_;}
      Vec operator()(double x) {return A*sin(om*x+phi)+offset;} 
  };  
  class FuncGainOffset : public UserFunction {
    protected:
      double a;
      Vec b;
      DataInterfaceBase *f;
    public:
      /** gain*(f(x)+offset)*/
      FuncGainOffset(DataInterfaceBase *f_,double gain_,const Vec& offset_);
      Vec operator()(double x) {return a*((*f)(x)+b);} 
  };  
  class FuncFunction : public UserFunction {
    protected:
      UserFunction *f;
      DataInterfaceBase *g;
    public:
      /** f(g(x)) nur für skalare Funktionen g(x) TODO, unsauber!!!
      */
      FuncFunction(UserFunction *f_, DataInterfaceBase *g_);
      Vec operator()(double x) {return (*f)(((*g)(x))(0));} 
  };
 
}

#endif
