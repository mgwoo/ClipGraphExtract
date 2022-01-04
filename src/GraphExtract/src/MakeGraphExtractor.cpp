#include <tcl.h>
#include "sta/StaMain.hh"
#include "ord/OpenRoad.hh"
#include "graphext/MakeGraphExtractor.h"
#include "graphext/GraphExtractor.h"


namespace sta {
extern const char *GraphExtractor_tcl_inits[];
}

extern "C" {
extern int Graphextractor_Init(Tcl_Interp* interp);
}

namespace ord {

GraphExtract::GraphExtractor * 
makeGraphExtractor() 
{
  return new GraphExtract::GraphExtractor; 
}

void 
initGraphExtractor(OpenRoad *openroad) 
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Graphextractor_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::GraphExtractor_tcl_inits);
  openroad->getGraphExtractor()->setDb(openroad->getDb());
  openroad->getGraphExtractor()->setSta(openroad->getSta());
}

void
deleteGraphExtractor(GraphExtract::GraphExtractor *graphext)
{
  delete graphext;
}


}
