SUBDIRS =  numerics utils integrators analysers contact_kinematics contours functions observers .
 
lib_LTLIBRARIES = libmbsim.la
libmbsim_la_LDFLAGS = -version-info @LIBMBSIM_VERSION_INFO@
libmbsim_la_SOURCES = \
element.cc\
body.cc\
constitutive_laws.cc\
contact.cc\
single_contact.cc\
contour.cc\
environment.cc\
frame.cc\
functions_contact.cc\
dynamic_system.cc\
group.cc\
dynamic_system_solver.cc\
isotropic_rotational_spring_damper.cc\
joint.cc\
gear.cc\
gearing.cc\
spring_damper.cc\
friction.cc\
extern_generalized_io.cc\
link.cc\
link_mechanics.cc\
kinetic_excitation.cc\
kinematic_excitation.cc\
mbsim_event.cc\
object.cc\
objectfactory.cc\
rigid_body.cc\
graph.cc\
constraint.cc \
observer.cc \
solver.cc

libmbsim_la_LIBADD = integrators/libintegrators.la
libmbsim_la_LIBADD += analysers/libanalysers.la 
libmbsim_la_LIBADD += utils/libutils.la 
libmbsim_la_LIBADD += contact_kinematics/libcontactkinematics.la 
libmbsim_la_LIBADD += contours/libcontour.la 
libmbsim_la_LIBADD += numerics/libnumerics.la 
libmbsim_la_LIBADD += functions/libfunction.la 
libmbsim_la_LIBADD += observers/libobserver.la 
libmbsim_la_LIBADD += -lgfortran $(DEPS_LIBS) $(OPENMBVCPPINTERFACE_LIBS)
libmbsim_la_LIBADD += -l@BOOST_SYSTEM_LIB@

libmbsim_la_CPPFLAGS = -I$(top_srcdir) $(DEPS_CFLAGS) $(OPENMBVCPPINTERFACE_CFLAGS)

mbsimincludedir = $(includedir)/mbsim
mbsiminclude_HEADERS = \
body.h\
contact.h\
single_contact.h\
contour.h\
contour_pdata.h\
constitutive_laws.h\
discretization_interface.h\
dynamic_system.h\
dynamic_system_solver.h\
element.h\
environment.h\
frame.h\
functions_contact.h\
group.h\
joint.h\
gear.h\
gearing.h\
isotropic_rotational_spring_damper.h\
spring_damper.h\
friction.h\
extern_generalized_io.h\
link.h\
link_mechanics.h\
kinetic_excitation.h\
kinematic_excitation.h\
mbsim_event.h\
modelling_interface.h\
object.h\
objectfactory.h\
rigid_body.h\
graph.h\
constraint.h \
observer.h \
solver.h
