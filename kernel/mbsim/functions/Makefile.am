noinst_LTLIBRARIES = libfunction.la
libfunction_la_SOURCES = basic_functions.cc \
			 tabular_functions.cc \
			 symbolic_functions.cc \
			 kinematic_functions.cc \
			 kinetic_functions.cc \
			 piecewise_polynom_function.cc \
			 fourier_function.cc

functionincludedir = $(includedir)/mbsim/functions

libfunction_la_LIBADD = $(DEPS_LIBS) $(OPENMBVCPPINTERFACE_LIBS) $(CASADI_LIBS)
libfunction_la_CPPFLAGS = -I$(top_srcdir) $(DEPS_CFLAGS) $(OPENMBVCPPINTERFACE_CFLAGS) $(CASADI_CFLAGS)

functioninclude_HEADERS = function.h \
			  basic_functions.h \
			  tabular_functions.h \
			  symbolic_functions.h \
			  kinematic_functions.h \
			  kinetic_functions.h \
			  auxiliary_functions.h \
			  piecewise_polynom_function.h \
			  fourier_function.h
