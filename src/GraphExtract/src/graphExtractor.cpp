#include "graphext/GraphExtractor.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbWireGraph.h"
#include "odb/dbObject.h"
#include "odb/wOrder.h"

#include "sta/Machine.hh"
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

//#include "sta/Levelize.hh"

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

// save point and dbNet* pointer to traverse later
typedef std::pair<box, odb::dbNet*> NetValue;
typedef bgi::rtree< NetValue, bgi::quadratic<6> > WireRTree;

using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::make_pair;
using std::pair;
using std::set;

namespace GraphExtract {

static int getInstancesNotInSet(odb::dbNet* net, std::set<odb::dbInst*> &instSet);
static void visitNeightbor(std::set<odb::dbInst*> &instSet, int depth,
    int netThreshold);

GraphExtractor::GraphExtractor() : 
  db_(nullptr), sta_(nullptr), rTree_(nullptr),
  wireRTree_(nullptr),
  graphModel_(Star), edgeWeightModel_(A), 
  visitDepth_(0), netThreshold_(INT_MAX),
  fakeEdgeWeight_(0.01), numFakeEdgesPerCC_(0), 
  fileName_(""),
  verbose_(1) {};

GraphExtractor::~GraphExtractor() {
  clear(); 
}

void
GraphExtractor::clear() {
  db_ = nullptr; 
  sta_ = nullptr;

  graphModel_ = Star;
  edgeWeightModel_ = A;
  visitDepth_ = 0;
  netThreshold_ = INT_MAX;
  fakeEdgeWeight_ = 0.0f;
  numFakeEdgesPerCC_ = 0;
  fileName_ = "";
}

void
GraphExtractor::clearRTrees() {
  if( rTree_ ) {
    delete (RTree*) rTree_;
  }
  rTree_ = nullptr;

  if( wireRTree_ ) {
    delete (WireRTree*) wireRTree_;
  }
  wireRTree_ = nullptr;
}

static void 
printITerm(odb::dbBlock* block, odb::dbObject* object) {
  odb::dbITerm* iTerm = odb::dbITerm::getITerm(block, object->getId());
  cout << iTerm->getInst()->getConstName() << "/" << iTerm->getMTerm()->getConstName();
  cout << " ";
  odb::Rect iTermBox = iTerm->getBBox();

  cout << ((iTerm->getIoType() == odb::dbIoType::OUTPUT)? "O" : 
      (iTerm->getIoType()==odb::dbIoType::INPUT)? "I" : " ");
  cout << " " << iTermBox.xMin() << " " << iTermBox.yMin();
  cout << " " << iTermBox.xMax() << " " << iTermBox.yMax();
}

static void
printBTerm(odb::dbBlock* block, odb::dbObject* object) {
  odb::dbBTerm* bTerm = odb::dbBTerm::getBTerm(block, object->getId());
  cout << bTerm->getConstName() << " " ;
  cout << ((bTerm->getIoType() == odb::dbIoType::OUTPUT)? "O" :
     (bTerm->getIoType()==odb::dbIoType::INPUT)? "I" : " ");

  odb::Rect bTermBox = bTerm->getBBox();
  cout << " " << bTermBox.xMin() << " " << bTermBox.yMin();
  cout << " " << bTermBox.xMax() << " " << bTermBox.yMax(); 
}

void
GraphExtractor::init() {  
  using namespace odb;
  dbBlock* block = db_->getChip()->getBlock();

  // RTree build for all insts -- need to be initialized only once!
  if( !rTree_ ) {
    rTree_ = (void*) (new RTree);
    RTree* rTree = (RTree*)rTree_;
  
    // DB Query to fill in RTree
    for( dbInst* inst : block->getInsts() ) {
      dbBox* bBox = inst->getBBox();
      box b (point(bBox->xMin(), bBox->yMin()), 
          point(bBox->xMax(), bBox->yMax()));
      rTree->insert( make_pair(b, inst) );
    }
  }

  // dbMaster mapping
  if( masterMap_.size() == 0 ) {
    int cnt = 0;
    for( dbInst* inst: block->getInsts() ) { 
      dbMaster* master = inst->getMaster();
      auto mmPtr = masterMap_.find( master );
      if( mmPtr == masterMap_.end() ) {
        masterMap_[ master ] = cnt++;
      }
    }
  }

  // RTree build for all wires -- need to be initialized only once!
  if( !wireRTree_ ) {
    wireRTree_ = (void*) (new WireRTree);
    WireRTree* wireRTree = (WireRTree*) wireRTree_;
  
    // odb::dbWireDecoder decoder; 
    // orderWires must be called to have connectivity in dbWireGraph
    odb::orderWires(block, nullptr, false, false, true);
    for( dbNet* net : block->getNets()) {
      dbWire* wire = net -> getWire();
//      cout << net->getConstName() << endl;
 
      // insert bTermBox into RTree 
      for( dbBTerm* bTerm : net->getBTerms() ) {
        odb::Rect rect = bTerm->getBBox();
        box b (point(rect.xMin(), rect.yMin()), 
          point(rect.xMax(), rect.yMax()));
        wireRTree->insert( make_pair(b, net));
      }

      // insert iTermBox into RTree
      for( dbITerm* iTerm : net->getITerms() ) {
        // skip for power/ground
        if( iTerm->getSigType() == odb::dbSigType::POWER ||
            iTerm->getSigType() == odb::dbSigType::GROUND ) {
          continue;
        }

        odb::Rect rect = iTerm->getBBox(); 
        box b (point(rect.xMin(), rect.yMin()), 
          point(rect.xMax(), rect.yMax()));
        wireRTree->insert( make_pair(b, net));
      } 

      if( wire ) {
        // dbWireGraph procedure
        odb::dbWireGraph graph;
        graph.decode(wire); 

        for(odb::dbWireGraph::edge_iterator edgeIter = graph.begin_edges(); 
            edgeIter != graph.end_edges(); 
            edgeIter ++) {

          odb::dbWireGraph::Edge* edge = *edgeIter;

          int x1 = -1, y1 = -1, x3 = -1, y3 = -1;
          odb::dbWireGraph::Node* source = edge->source();
          odb::dbWireGraph::Node* target = edge->target();

          source->xy(x1, y1);
          target->xy(x3, y3);

          // cout << "[EDGE] Type: " << edge->type() << " coordi: " << x1 << " " << y1 << " " << x3 << " " << y3 << endl;

          // insert bbox into wireRTree 
          box b (point(std::min(x1,x3), std::min(y1,y3)), 
              point(std::max(x1,x3), std::max(y1,y3)));
          wireRTree->insert( make_pair(b, net));

          // odb::dbObject* sourceObject = source->object();
          // if( sourceObject ) {
          //   cout << "       Source: " << sourceObject->getObjName() << " ";
          //   if( sourceObject->getObjName() == "dbITerm" ) {
          //     printITerm(block, sourceObject);
          //   }
          //   else if( sourceObject->getObjName() == "dbBTerm") { 
          //     printBTerm(block, sourceObject);
          //   }
          //   cout << endl;
          // }

          // odb::dbObject* targetObject = target->object();
          // if( targetObject ) {
          //   cout << "       Target: " << targetObject->getObjName() << " ";
          //   if( targetObject->getObjName() == "dbITerm" ) { 
          //     printITerm(block, targetObject);
          //   }
          //   else if( targetObject->getObjName() == "dbBTerm") {
          //     printBTerm(block, targetObject);
          //   }
          //   cout << endl;
          // }
        }
      }
    }
  }
}



// overlap checker on two rectangles
// true -> overlap // false -> no overlap
static bool 
isLineOverlap(odb::Rect& rect1, odb::Rect& rect2) {
  int x1 = std::max(rect1.xMin(), rect2.xMin());
  int x3 = std::min(rect1.xMax(), rect2.xMax());

  int y1 = std::max(rect1.yMin(), rect2.yMin());
  int y3 = std::min(rect1.yMax(), rect2.yMax());

  return !(x1 > x3 || y1 > y3);
}

// note that rect2 is bigger
static bool
isWithin(odb::Rect& rect1, odb::Rect& rect2) {
  if( rect2.xMin() < rect1.xMin() && rect2.xMax() > rect1.xMax()
      && rect2.yMin() < rect1.yMin() && rect2.yMax() > rect1.yMax() ) {
    return true;
  }
  else {
    return false;
  }
}

bool 
GraphExtractor::extract(int lx, int ly, int ux, int uy) {
  odb::Rect clipRect(lx, ly, ux, uy);

  RTree* rTree = (RTree*)rTree_;
  WireRTree* wireRTree = (WireRTree*)wireRTree_;

  //sta_->updateTiming(false);
  //sta::dbNetwork* network = sta_->getDbNetwork();
  //sta::Graph* graph = sta_->ensureGraph();
  //sta_->ensureLevelized();
  //cout << "MaxLevel: " << sta_->levelize()->maxLevel() << endl;
  
  box queryBox( point(lx, ly), point(ux, uy) );

  //GraphExtSrchPred graphExtSrchPred(sta_);


  vector<NetValue> foundNets;
  wireRTree->query(bgi::intersects(queryBox),
      std::back_inserter(foundNets));

  int idx = 0; 
  set<odb::dbNet*> netSet;  
  set<odb::dbInst*> instSet;

  set<odb::dbNet*> netSearchDupChk;

  // extract insts!
  for(NetValue & val : foundNets) {
    odb::dbNet* net = val.second;
    // already considered -- save runtime
    if( netSearchDupChk.find( net ) != netSearchDupChk.end() ) {
      continue;
    }

    netSearchDupChk.insert(net);

    if( net->getBTerms().size() + net->getITerms().size() <= 1) { 
      if( verbose_ > 1 ) {
        cout << "#inst < 1 net detected. skip for net: " 
          << net->getConstName() << endl;
      }
      continue;
    }

    if( net->getSigType() == odb::dbSigType::POWER ||
        net->getSigType() == odb::dbSigType::GROUND ) {
      if( verbose_ > 1 ) {
        cout << "power/ground net detected. skip for net: "
          << net->getConstName() << endl;
      }
      continue;
    }

    // check for dbBTerm and dbITerm
    bool isVertexInside = false;

    vector<odb::dbBTerm*> inBTerms;
    vector<odb::dbITerm*> inITerms;
    for(odb::dbBTerm* bTerm : net->getBTerms()) {
      odb::Rect bRect = bTerm->getBBox();
      if( isLineOverlap(bRect, clipRect) ) {
        inBTerms.push_back(bTerm);
        isVertexInside = true;
      }
    }

    for(odb::dbITerm* iTerm : net->getITerms()) {
      odb::Rect iRect = iTerm->getBBox();
      if( isLineOverlap(iRect, clipRect) ) {
        inITerms.push_back(iTerm);
        isVertexInside = true;
      }
    }

    if( !isVertexInside ) {
//      cout << "No inst/points inside clip. skip for net: " 
//        << net->getConstName() << endl;
      continue;
    }
    
    // 
    for(auto iTerm : inITerms) {
      instSet.insert( iTerm->getInst() ); 
    } 
    
    netSet.insert( net );
  }
 
  // skip for empty inst clips 
  if( instSet.size() == 0 ) {
    if( verbose_ > 1 ) {
      cout << "No insts" << endl;
    }
    return false;
  }

  if( verbose_ > 1 ) {
    cout << "NumFoundInsts: " << instSet.size() << endl;
  }

  Graph instGraph;
  instGraph.setDb(db_);

  // hand over insts
  bool success = instGraph.init(netSet,
      instSet, 
      &clipRect,
      graphModel_, edgeWeightModel_,
      verbose_ );
  if( !success ) {
    return false;
  }

  instGraph.connectIsolatedClusters(fakeEdgeWeight_, 
      numFakeEdgesPerCC_);
  //instGraph.printEdgeList();
  instGraph.saveFile(fileName_);
  if( masterFileName_ != "") {
    instGraph.saveMasterFile(masterFileName_, masterMap_);
  }
  if( verbose_ > 1 ) {
    cout << "Done!" << endl << endl;
  }
  return true;
}

void
GraphExtractor::setDb(odb::dbDatabase* db) {
  db_ = db;
}

void
GraphExtractor::setSta(sta::dbSta* sta) {
  sta_ = sta;
}

void
GraphExtractor::setGraphModel(const char* graphModel) {
  if( strcmp(graphModel, "star") == 0 ) {
    graphModel_ = Star; 
  }
  else if( strcmp(graphModel, "clique") == 0 ) {
    graphModel_ = Clique;
  }
}

void
GraphExtractor::setVisitDepth(int visitDepth) {
  visitDepth_ = visitDepth;
}

void
GraphExtractor::setNetThreshold(int threshold) {
  netThreshold_ = threshold;
}

void
GraphExtractor::setFakeEdgeWeight(float weight) {
  fakeEdgeWeight_ = weight; 
}

void
GraphExtractor::setNumFakeEdgesPerCC(int num) {
  numFakeEdgesPerCC_ = num;
}

void
GraphExtractor::setVerbose(int verbose) {
  verbose_ = verbose;
}

void
GraphExtractor::setSaveFileName(const char* fileName) {
  fileName_ = fileName;
}

void
GraphExtractor::setSaveMasterFileName(const char* fileName) {
  masterFileName_ = fileName; 
}

// TODO
void
GraphExtractor::setEdgeWeightModel( const char* edgeWeightModel ) {
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
  else {
    cout << "ERROR: edgeWeight is wrong: " << edgeWeightModel << endl;
    exit(1);
  }
}

static int getInstancesNotInSet(odb::dbNet* net, 
    std::set<odb::dbInst*> &instSet) {

  std::set<odb::dbInst*> visitInstSet;

  // avoid duplicated instance
  for(odb::dbITerm* iTerm : net->getITerms()) {
    visitInstSet.insert(iTerm->getInst());
  }

  // increase cnt
  // if not in given set
  int cnt = 0;
  for(auto& inst : visitInstSet) {
    if( instSet.find(inst) == instSet.end() ) {
      cnt++;
    }
  }
  return cnt;
}

static void visitNeightbor(std::set<odb::dbInst*> &instSet, int depth, int netThreshold) {
  using namespace odb;
  // repeat until depth reached
  for(int i=0; i<depth; i++) {
    set<dbITerm*> iTermSet;
    for(auto& inst: instSet) {
      for(dbITerm* iTerm : inst->getITerms()) {
        if( iTerm->getSigType() == odb::dbSigType::POWER ||
            iTerm->getSigType() == odb::dbSigType::GROUND ) {
          continue;
        }
        if( strcmp(iTerm->getMTerm()->getConstName(), "SI") == 0 ||
            strcmp(iTerm->getMTerm()->getConstName(), "SE") == 0 ) {
          continue;
        }
        iTermSet.insert(iTerm);
      }
    }

    set<dbNet*> netSet;
    for(auto& iTerm : iTermSet) {
      dbNet* net = iTerm->getNet();
      if( !net ) {
        continue;
      }
      if (net->getSigType() == dbSigType::POWER 
          || net->getSigType() == dbSigType::GROUND
          || net->getSigType() == dbSigType::CLOCK) {
        continue;
      }
 
      // escape searching when
      // #instance not in set is larger than
      // netThreshold
      if( netThreshold != INT_MAX 
          && getInstancesNotInSet( net, instSet ) >= netThreshold ) {
        continue;
      }
      netSet.insert(iTerm->getNet());
    }

    for(auto& net : netSet) {
      for(dbITerm* iTerm : net->getITerms()) {
        if( iTerm->getSigType() == odb::dbSigType::POWER ||
            iTerm->getSigType() == odb::dbSigType::GROUND ) {
          continue;
        }
        if( strcmp(iTerm->getMTerm()->getConstName(), "SI") == 0 ||
            strcmp(iTerm->getMTerm()->getConstName(), "SE") == 0 ) {
          continue;
        }
        instSet.insert(iTerm->getInst());
      }
    }
  }
}

}
