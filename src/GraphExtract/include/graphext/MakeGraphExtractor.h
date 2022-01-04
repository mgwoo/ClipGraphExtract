
#ifndef MAKE_GRAPH_EXTRACTOR
#define MAKE_GRAPH_EXTRACTOR

namespace GraphExtract {
class GraphExtractor;
}

namespace ord {

class OpenRoad;

GraphExtract::GraphExtractor*
makeGraphExtractor();

void
initGraphExtractor(OpenRoad *openroad);

void
deleteGraphExtractor(GraphExtract::GraphExtractor *graphext);

}

#endif
