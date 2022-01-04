include("openroad")

# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SWIG REQUIRED)
find_package(Boost REQUIRED)

#set(GRAPHEXT_WRAP ${CMAKE_CURRENT_BINARY_DIR}/graphExtractor_wrap.cc)
#set(GRAPHEXT_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/GraphExtTclInitVar.cc)

# add_custom_command(OUTPUT ${GRAPHEXT_WRAP}
#   COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -I${GRAPHEXT_HOME}/include -I${OPENDB_HOME}/src/swig/tcl -o ${GRAPHEXT_WRAP} ${GRAPHEXT_HOME}/src/graphExtractor.i
#   WORKING_DIRECTORY ${GRAPHEXT_HOME}/src
#   DEPENDS ${GRAPHEXT_HOME}/src/graphExtractor.i ${GRAPHEXT_HOME}/include/graphext/GraphExtractor.h
# )
# 
# add_custom_command(OUTPUT ${GRAPHEXT_TCL_INIT}
#   COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${GRAPHEXT_TCL_INIT} graph_extractor_tcl_inits ${GRAPHEXT_HOME}/src/graphExtractor.tcl
#   WORKING_DIRECTORY ${GRAPHEXT_HOME}/src
#   DEPENDS ${GRAPHEXT_HOME}/src/graphExtractor.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
#   )

swig_lib(NAME   GraphExtractor
          NAMESPACE GraphExtractor
          I_FILE  src/graphExtractor.i
          SCRIPTS src/graphExtractor.tcl
)

swig_lib(NAME   GraphExtractor_py
          NAMESPACE GraphExtractor
          LANGUAGE python
          I_FILE  src/graphExtractor_py.i
          SCRIPTS ${CMAKE_CURRENT_BINARY_DIR}/GraphExtractor_py.py
)

target_include_directories( GraphExtractor_py
  PUBLIC include
  PRIVATE 
  ${GRAPHEXT_HOME}/src/
)


############################################################
# Libraries setting 
############################################################

target_sources( GraphExtractor
  PRIVATE
    src/graphExtractor.cpp
    src/MakeGraphExtractor.cpp
    src/instGraph.cpp
    #${GRAPHEXT_WRAP}
    #${GRAPHEXT_TCL_INIT}
)

target_link_libraries( GraphExtractor 
  PRIVATE
    utl
    odb
    OpenSTA
    dbSta
)


target_include_directories( GraphExtractor
  PUBLIC include
  PRIVATE
  ${GRAPHEXT_HOME}/src
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/app
  ${OPENSTA_HOME}/dcalc
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/sdf
  ${OPENSTA_HOME}/search
  ${OPENSTA_HOME}/tcl
  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/verilog
  ${OPENSTA_HOME}/search
  ${OPENSTA_HOME}/liberty
  ${Boost_INCLUDE_DIR}
 )
