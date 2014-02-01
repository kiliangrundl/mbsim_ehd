/* Copyright (C) 2004-2009 MBSim Development Team
 *
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
 * Contact: martin.o.foerg@googlemail.com
 */

#ifndef UTILS_H_
#define UTILS_H_

#define PVNS_ "http://openmbv.berlios.de/MBXMLUtils/physicalvariable"
#define PVNS "{"PVNS_"}"

#include <string>
#include "fmatvec/function.h"
#include "mbxmlutilstinyxml/tinyxml.h"
#include <limits>
#include <vector>
#include <set>

namespace MBSim {

  std::string numtostr(int i);   
  std::string numtostr(double d);   
  template<class Type, class Row, class Col>
    std::string numtostr(fmatvec::Matrix<Type,Row,Col,double> m) {
      std::ostringstream oss;
      oss << "[ ";
      for(int i=0;i<m.rows()-1;i++) {
        for(int j=0;i<m.cols()-1;j++) oss << m.e(i,j) << ", ";
        oss << "\n";
      }
      oss << "]";
      return oss.str(); 
    }

  double degtorad(double alpha);
  double radtodeg(double phi);
  fmatvec::Vec degtorad(fmatvec::Vec alpha);
  fmatvec::Vec radtodeg(fmatvec::Vec phi);
  fmatvec::Vec tildetovec(const fmatvec::SqrMat &A);

  double sign(double x);

  /*!
   * \brief calculates planar angle in [0,2\pi] with respect to Cartesian coordinates of: Arc Tangent (y/x)
   * \param Cartesian x-coordinate
   * \param Cartesian y-coordinate
   * \return angle
   */
  double ArcTan(double x,double y);

  template <class T>
    inline std::string toStr(const T &val) {
      std::stringstream s;
      s << std::setprecision(std::numeric_limits<double>::digits10+1) << val;
      return s.str();
    }

  inline MBXMLUtils::TiXmlNode* toXML(const std::string &str) {
    return new MBXMLUtils::TiXmlText(str);
  }

  inline MBXMLUtils::TiXmlNode* toXML(int i) {
    return new MBXMLUtils::TiXmlText(toStr(i));
  }

  inline MBXMLUtils::TiXmlNode* toXML(unsigned int i) {
    return new MBXMLUtils::TiXmlText(toStr(i));
  }

  inline MBXMLUtils::TiXmlNode* toXML(double d) {
    return new MBXMLUtils::TiXmlText(toStr(d));
  }

  template <class T>
    inline MBXMLUtils::TiXmlNode* toXML(const std::vector<T> &x) {
      MBXMLUtils::TiXmlElement *ele = new MBXMLUtils::TiXmlElement(PVNS"xmlVector");
      for(unsigned int i=0; i<x.size(); i++) {
        MBXMLUtils::TiXmlElement *elei = new MBXMLUtils::TiXmlElement(PVNS"ele");
        MBXMLUtils::TiXmlText *text = new MBXMLUtils::TiXmlText(toStr(x[i]));
        elei->LinkEndChild(text);
        ele->LinkEndChild(elei);
      }
      return ele;
    }

  template <class Row>
    inline MBXMLUtils::TiXmlNode* toXML(const fmatvec::Vector<Row,double> &x) {
      MBXMLUtils::TiXmlElement *ele = new MBXMLUtils::TiXmlElement(PVNS"xmlVector");
      for(int i=0; i<x.size(); i++) {
        MBXMLUtils::TiXmlElement *elei = new MBXMLUtils::TiXmlElement(PVNS"ele");
        MBXMLUtils::TiXmlText *text = new MBXMLUtils::TiXmlText(toStr(x.e(i)));
        elei->LinkEndChild(text);
        ele->LinkEndChild(elei);
      }
      return ele;
    }

  template <class Type, class Row, class Col>
    inline MBXMLUtils::TiXmlNode* toXML(const fmatvec::Matrix<Type,Row,Col,double> &A) {
      MBXMLUtils::TiXmlElement *ele = new MBXMLUtils::TiXmlElement(PVNS"xmlMatrix");
      for(int i=0; i<A.rows(); i++) {
        MBXMLUtils::TiXmlElement *elei = new MBXMLUtils::TiXmlElement(PVNS"row");
        for(int j=0; j<A.cols(); j++) {
          MBXMLUtils::TiXmlElement *elej = new MBXMLUtils::TiXmlElement(PVNS"ele");
          MBXMLUtils::TiXmlText *text = new MBXMLUtils::TiXmlText(toStr(A.e(i,j)));
          elej->LinkEndChild(text);
          elei->LinkEndChild(elej);
        }
        ele->LinkEndChild(elei);
      }
      return ele;
    }

  template<class T>
    inline std::string funcExt() {
      return "V";
    }

  template < >
    inline std::string funcExt<double>() {
      return "S";
    }

  template <class T>
    void addElementText(MBXMLUtils::TiXmlElement *parent, std::string name, const T &value) {
      parent->LinkEndChild(new MBXMLUtils::TiXmlElement(name))->LinkEndChild(toXML(value));
    }

  template <class Arg>
    class ToDouble {
    };

  template <>
    class ToDouble<double> {
      public:
        static double cast(const double &x) {
          return x;
        }
    };

  template <class Col>
    class ToDouble<fmatvec::Vector<Col,double> > {
      public:
        static double cast(const fmatvec::Vector<Col,double> &x) {
          return x.e(0); 
        }
    };

  template <class Ret>
  class FromMatStr {
    public:
      static Ret cast(const char *x) {
        return Ret(x);
      }
  };

  template <>
  class FromMatStr<double> {
    public:
      static double cast(const char *x) {
        return atof(x);
      }
  };

  template <class Ret>
  class FromDouble {
    public:
      static Ret cast(double x) {
        return Ret(1,fmatvec::INIT,x);
      }
  };

  template <>
  class FromDouble<double> {
    public:
      static double cast(double x) {
        return x;
      }
  };

  template <class Ret>
  class FromVecV {
    public:
      static Ret cast(const fmatvec::VecV &x) {
        return x;
      }
  };

  template <>
  class FromVecV<double> {
    public:
      static double cast(const fmatvec::VecV &x) {
        return x(0);
      }
  };

  template <class Ret>
    class StateDependentFunction : public fmatvec::Function<Ret(fmatvec::VecV,double)> {
      private:
        fmatvec::Function<Ret(fmatvec::VecV)> *f;
      public:
        StateDependentFunction(fmatvec::Function<Ret(fmatvec::VecV)> *f_=NULL) : f(f_) { }
        ~StateDependentFunction() { delete f; }
        typename fmatvec::Size<fmatvec::VecV>::type getArg1Size() const { return f->getArgSize();}
        typename fmatvec::Size<double>::type getArg2Size() const { return 0; }
        Ret operator()(const fmatvec::VecV &arg1, const double &arg2) {return (*f)(arg1); }
        typename fmatvec::Der<Ret, fmatvec::VecV>::type parDer1(const fmatvec::VecV &arg1, const double &arg2) { return f->parDer(arg1); }
        typename fmatvec::Der<Ret, double>::type parDer2(const fmatvec::VecV &arg1, const double &arg2) {return fmatvec::Vec3(); }
        typename fmatvec::Der<typename fmatvec::Der<Ret, double>::type, double>::type parDer2ParDer2(const fmatvec::VecV &arg1, const double &arg2) { return fmatvec::Vec3(); }
        typename fmatvec::Der<Ret, double>::type parDer2DirDer1(const fmatvec::VecV &arg1Dir, const fmatvec::VecV &arg1, const double &arg2) { return fmatvec::Vec3(); }
        typename fmatvec::Der<typename fmatvec::Der<Ret, fmatvec::VecV>::type, double>::type parDer1ParDer2(const fmatvec::VecV &arg1, const double &arg2) { return fmatvec::Mat3xV(getArg1Size()); }
        typename fmatvec::Der<Ret, fmatvec::VecV>::type parDer1DirDer1(const fmatvec::VecV &arg1Dir, const fmatvec::VecV &arg1, const double &arg2) { return f->parDerDirDer(arg1Dir,arg1); }
        bool constParDer1() const { return f->constParDer(); }
        bool constParDer2() const { return true; }
        fmatvec::Function<Ret(fmatvec::VecV)>* getFunction() const { return f; }
    };

  template <class Ret>
    class TimeDependentFunction : public fmatvec::Function<Ret(fmatvec::VecV,double)> {
      private:
        fmatvec::Function<Ret(double)> *f;
      public:
        TimeDependentFunction(fmatvec::Function<Ret(double)> *f_=NULL) : f(f_) { }
        ~TimeDependentFunction() { delete f; }
        typename fmatvec::Size<fmatvec::VecV>::type getArg1Size() const { return 0;}
        typename fmatvec::Size<double>::type getArg2Size() const { return 1; }
        Ret operator()(const fmatvec::VecV &arg1, const double &arg2) {return (*f)(arg2); }
        typename fmatvec::Der<Ret, fmatvec::VecV>::type parDer1(const fmatvec::VecV &arg1, const double &arg2) { return fmatvec::Mat3xV(); }
        typename fmatvec::Der<Ret, double>::type parDer2(const fmatvec::VecV &arg1, const double &arg2) {return f->parDer(arg2); }
        typename fmatvec::Der<typename fmatvec::Der<Ret, double>::type, double>::type parDer2ParDer2(const fmatvec::VecV &arg1, const double &arg2) { return f->parDerParDer(arg2); }
        typename fmatvec::Der<Ret, double>::type parDer2DirDer1(const fmatvec::VecV &arg1Dir, const fmatvec::VecV &arg1, const double &arg2) { return fmatvec::Vec3(); }
        typename fmatvec::Der<typename fmatvec::Der<Ret, fmatvec::VecV>::type, double>::type parDer1ParDer2(const fmatvec::VecV &arg1, const double &arg2) { return fmatvec::Mat3xV(); }
        typename fmatvec::Der<Ret, fmatvec::VecV>::type parDer1DirDer1(const fmatvec::VecV &arg1Dir, const fmatvec::VecV &arg1, const double &arg2) { return fmatvec::Mat3xV(); }
        bool constParDer1() const { return true; }
        bool constParDer2() const { return f->constParDer(); }
    };

}

#endif