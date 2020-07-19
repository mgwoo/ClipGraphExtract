#include "clip_graph_ext/clipGraphExtractor.h"
#include "opendb/db.h"

#include "sta/Graph.hh"
#include "sta/Sta.hh"
#include "sta/Network.hh"
#include "sta/Liberty.hh"
#include "sta/Sdc.hh"
#include "sta/PortDirection.hh"
#include "sta/Corner.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathEnd.hh"
#include "sta/PathRef.hh"
#include "sta/Search.hh"
#include "sta/Bfs.hh"

#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "instGraph.h"
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// for easier coding with boost
typedef bg::model::point<int, 2, bg::cs::cartesian> point;
typedef bg::model::box<point> box;

// save point and dbInst* pointer 
typedef std::pair<box, odb::dbInst*> value;
typedef bgi::rtree< value, bgi::quadratic<6> > RTree;

using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using std::set;

namespace ClipGraphExtract {

ClipGraphExtractor::ClipGraphExtractor() : 
  db_(nullptr), sta_(nullptr), rTree_(nullptr),
  graphModel_(Star), edgeWeightModel_(A), 
  fileName_("") {};

ClipGraphExtractor::~ClipGraphExtractor() {
  clear(); 
}

void
ClipGraphExtractor::clear() {
  db_ = nullptr; 
  sta_ = nullptr;
  if( rTree_ ) {
    delete (RTree*) rTree_;
  }
  rTree_ = nullptr;
  graphModel_ = Star;
  edgeWeightModel_ = A;
  fileName_ = "";
}

void 
ClipGraphExtractor::init() {  
  using namespace odb;
  rTree_ = (void*) (new RTree);
  RTree* rTree = (RTree*)rTree_;

  // DB Query to fill in RTree
	dbBlock* block = db_->getChip()->getBlock();
  for( dbInst* inst : block->getInsts() ) {
    dbBox* bBox = inst->getBBox();
    box b (point(bBox->xMin(), bBox->yMin()), 
        point(bBox->xMax(), bBox->yMax()));
    rTree->insert( make_pair(b, inst) );
  }

}

void
ClipGraphExtractor::extract(int lx, int ly, int ux, int uy) {
  RTree* rTree = (RTree*)rTree_;
  sta_->updateTiming(false);
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta::Graph* graph = sta_->ensureGraph();
  
  box queryBox( point(lx, ly), point(ux, uy) );

  vector<value> foundInsts; 
  rTree->query(bgi::intersects(queryBox), 
      std::back_inserter(foundInsts));

  cout << "NumFoundInsts: " << foundInsts.size() << endl;

  set<odb::dbInst*> instSet;
  for(value& val : foundInsts) {
    odb::dbInst* inst = val.second;
    instSet.insert( inst ); 
  }
  
  Graph instGraph;
  instGraph.setDb(db_);
  instGraph.init(instSet, graphModel_, edgeWeightModel_);
  instGraph.saveFile(fileName_);
  cout << "Done!" << endl;
}

void
ClipGraphExtractor::setDb(odb::dbDatabase* db) {
  db_ = db;
}

void
ClipGraphExtractor::setSta(sta::dbSta* sta) {
  sta_ = sta;
}

void
ClipGraphExtractor::setGraphModel(const char* graphModel) {
  if( strcmp(graphModel, "star") == 0 ) {
    graphModel_ = Star; 
  }
  else if( strcmp(graphModel, "clique") == 0 ) {
    graphModel_ = Clique;
  }
}

void
ClipGraphExtractor::setSaveFileName(const char* fileName) {
  fileName_ = fileName;
}

void
ClipGraphExtractor::setEdgeWeightModel( const char* edgeWeightModel ) {
  if( strcmp(edgeWeightModel, "a") == 0 ) {
    edgeWeightModel_ = A;
  }
  else if( strcmp(edgeWeightModel, "b") == 0 ) {
    edgeWeightModel_ = B;
  }
  else if( strcmp(edgeWeightModel, "c") == 0 ) {
    edgeWeightModel_ = C;
  }
  else if( strcmp(edgeWeightModel, "d") == 0 ) {
    edgeWeightModel_ = D;
  }
  else if( strcmp(edgeWeightModel, "e") == 0 ) {
    edgeWeightModel_ = E;
  }
  else {
    cout << "ERROR: edgeWeight is wrong: " << edgeWeightModel << endl;
    exit(1);
  }
}

}
