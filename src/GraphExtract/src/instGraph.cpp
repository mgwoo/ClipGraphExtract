#include "odb/db.h"
#include "odb/dbWireGraph.h"
#include "instGraph.h"

#include <set>
#include <cmath>
#include <fstream>
#include <queue>

using std::set;
using std::vector;
using std::cout;
using std::endl;
using std::make_pair;
using std::queue;

namespace GraphExtract {

Pin::Pin() 
  : type_(Type::TYPENONE), io_(IO::IONONE), 
  iTerm_(nullptr), bTerm_(nullptr),
  isOutside_(false) {};

Pin::Pin(odb::dbITerm* iTerm, bool isOutside) : Pin() {
  iTerm_ = iTerm;
  type_ = Type::ITERM;
  if( iTerm->getIoType() == odb::dbIoType::INPUT ) {
    io_ = IO:: INPUT;
  }
  else if( iTerm->getIoType() == odb::dbIoType::OUTPUT) { 
    io_ = IO:: OUTPUT;
  }
  isOutside_ = isOutside;
}

Pin::Pin(odb::dbBTerm* bTerm, bool isOutside) : Pin() {
  bTerm_ = bTerm;
  type_ = Type::BTERM;
  if( bTerm->getIoType() == odb::dbIoType::INPUT) { 
    io_ = IO:: OUTPUT;
  }
  else if( bTerm->getIoType() == odb::dbIoType::OUTPUT) { 
    io_ = IO:: INPUT;
  }
  isOutside_ = isOutside;
}

void Pin::setIo(IO io) {
  io_ = io;
}

void Pin::setType(Type type) {
  type_ = type;
}

void Pin::setIsOutside(bool isOutside) {
  isOutside_ = isOutside;
}

void Pin::print() {
  if( isOutside_ ) {
    cout << "OU ";
  }
  else {
    cout << "IN ";
  }

  if( type_ == Pin::Type::BTERM ) {
    cout << bTerm_->getConstName() << " "; 
  }
  else if( type_ == Pin::Type::ITERM) {
    cout << iTerm_->getInst()->getConstName() << "/" << iTerm_->getMTerm()->getConstName() << " ";
  }
  
  if( io_ == Pin::IO::OUTPUT) { 
    cout << "O ";
  }
  else if( io_ == Pin::IO::INPUT) {
    cout << "I ";
  }

  odb::Rect rect;
  if( type_ == Pin::Type::BTERM  ) {
    rect = bTerm_->getBBox();
  }
  else if( type_ == Pin::Type::ITERM ) {
    rect = iTerm_->getBBox();
  }

  cout << rect.xMin() << " " << rect.yMin () << " " 
    << rect.xMax() << " " << rect.yMax() << endl;
}

Vertex::Vertex() : 
  inst_(nullptr), 
  bTerm_(nullptr),
  type_(Vertex::NONE),
  weight_(0),
  id_(0),
  lx_(0),ly_(0),ux_(0),uy_(0) {};

// for instance Vertex
Vertex::Vertex(odb::dbInst* inst,
    int id,  
    float weight) 
  : Vertex() {
  inst_ = inst;
  id_ = id;
  weight_ = weight;
  type_ = Vertex::INST;
  inst->getLocation(lx_, ly_);
  
  ux_ = lx_ + inst->getMaster()->getWidth();
  uy_ = ly_ + inst->getMaster()->getHeight();
} 

// for IO port vertex
Vertex::Vertex(odb::dbBTerm* bTerm,
    int id, 
    float weight)
  : Vertex() {
  bTerm_ = bTerm;
  id_ = id;
  weight_ = weight;
  type_ = Vertex::IOPORT;

  odb::Rect rect = bTerm->getBBox();
  lx_ = rect.xMin();
  ly_ = rect.yMin();
  ux_ = rect.xMax();
  uy_ = rect.yMax();
}

// for boundary Vertex
Vertex::Vertex(int cx, int cy, 
    int id, 
    float weight)
  : Vertex() {
  weight_ = weight;
  id_ = id;
  type_ = Type::VIRTUAL;
  lx_ = ux_ = cx;
  ly_ = uy_ = cy;
}

void Vertex::setWeight(float weight) {
  weight_ = weight;
}

void Vertex::addInEdge(Edge* edge) {
  inEdges_.push_back(edge);
}

void Vertex::addOutEdge(Edge* edge) { 
  outEdges_.push_back(edge);
}


void Vertex::setLocation(int lx, int ly,
    int ux, int uy) {
  lx_ = lx; 
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

std::string Vertex::name() {
  if( type_ == INST ) { 
    return inst_->getConstName(); 
  }
  else if( type_ == IOPORT ) {
    return bTerm_->getConstName();
  }
  else if( type_ == VIRTUAL ) {
    return "VIRTUAL_" + std::to_string( id_ );
  } 
  return "NONE";
}

Edge::Edge() : 
  from_(nullptr), to_(nullptr), 
  weight_(0) {}

Edge::Edge(Vertex* from, 
    Vertex* to,
    float weight)
  : from_(from), to_(to), weight_(weight) {}

void Edge::setFrom(Vertex* vertex) {
  from_ = vertex;
}

void Edge::setTo(Vertex* vertex) {
  to_ = vertex;
}

void Edge::setWeight(float weight) {
  weight_ = weight;
}




Graph::Graph() : db_(nullptr) {}
void Graph::setDb(odb::dbDatabase* db) {
  db_ = db;
}

Graph::~Graph() {
  db_ = nullptr;
  vector<Vertex>().swap(vertices_);
  vector<Edge>().swap(edges_);
  vertexInstMap_.clear();
  vertexBTermMap_.clear();
}

static float getEdgeWeight(int fanout, EdgeWeightModel eModel) {
  switch( eModel ) {
    case A:
      return 4.0/(fanout*(fanout-1));
      break;
    case B:
      return 2.0/fanout;
      break;
    case C:
      return 8.0/(fanout*fanout*fanout);
      break;
    case D:
      return 2.0/(std::pow(fanout,1.5));
      break;
    default:
      return 1.0/(fanout-1);
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

static std::pair<int, int> 
getCxCyEntry(
    int x1, int y1, 
    int x3, int y3, 
    odb::Rect& clipRect)  {
  odb::Rect edgeRect(x1,y1,x3,y3); 
  int cx=-1, cy=-1;

  if( isLineOverlap(edgeRect, clipRect) ) {
//    cout << "overlap" << endl;
    // HORIZONTAL
    if( abs(x3-x1) > abs(y3-y1) ) {
      // LEFT to RIGHT
      if( x3 > x1 ) {
        // LEFT boundary: entry
        if( x1 <= clipRect.xMin() &&
            clipRect.xMin() <= x3 ) {
          cx = clipRect.xMin();
          cy = (y1+y3)/2;
        }
        // should not happen
        // else if( x1 <= clipRect.xMax() &&
        //     clipRect.xMax() <= x3 ) {
        //   cx = clipRect.xMax();
        //   cy = (y1+y3)/2;
        // }
      }
      // RIGHT to LEFT
      else {
        // RIGHT boundary 
        if( x3 <= clipRect.xMax() &&
          clipRect.xMax() <= x1 ) {
          cx = clipRect.xMax();
          cy = (y1+y3)/2;
        }
        // should not happen
      }
    }
    // VERTICAL 
    else {
      // DOWN to UP
      if( y3 > y1 ) {
        // BOTTOM boundary: entry
        if( y1 <= clipRect.yMin() &&
            clipRect.yMin() <= y3 ) {
          cx = (x1+x3)/2;
          cy = clipRect.yMin();
        }
        // should not happen 
      }
      // UP to DOWN
      else {
        // UP boundary: entry
        if( y3 <= clipRect.yMax() &&
            clipRect.yMax() <= y1 ) {
          cx = (x1+x3)/2;
          cy = clipRect.yMax();
        }
        // should not happen
      }
    }
  }
  else {
//    cout << "not overlap" << endl;
  }
  return make_pair(cx, cy);
}

static std::pair<int, int> 
getCxCyExit(
    int x1, int y1, 
    int x3, int y3, 
    odb::Rect& clipRect)  {
  odb::Rect edgeRect(x1,y1,x3,y3); 
  int cx=-1, cy=-1;

  if( isLineOverlap(edgeRect, clipRect) ) {
//    cout << "overlap" << endl;
    // HORIZONTAL
    if( abs(x3-x1) > abs(y3-y1) ) {
      // LEFT to RIGHT
      if( x3 > x1 ) {
        // RIGHT boundary: exit  
        if( x1 <= clipRect.xMax() &&
            clipRect.xMax() <= x3 ) {
          cx = clipRect.xMax();
          cy = (y1+y3)/2;
        }
        // should not happen
        // else if( x1 <= clipRect.xMax() &&
        //     clipRect.xMax() <= x3 ) {
        //   cx = clipRect.xMax();
        //   cy = (y1+y3)/2;
        // }
      }
      // RIGHT to LEFT
      else {
        // LEFT boundary 
        if( x3 <= clipRect.xMin() &&
          clipRect.xMin() <= x1 ) {
          cx = clipRect.xMin();
          cy = (y1+y3)/2;
        }
        // should not happen
      }
    }
    // VERTICAL 
    else {
      // DOWN to UP
      if( y3 > y1 ) {
        // UP boundary: exit 
        if( y1 <= clipRect.yMax() &&
            clipRect.yMax() <= y3 ) {
          cx = (x1+x3)/2;
          cy = clipRect.yMax();
        }
        // should not happen 
      }
      // UP to DOWN
      else {
        // DOWN boundary: 
        if( y3 <= clipRect.yMin() &&
            clipRect.yMin() <= y1 ) {
          cx = (x1+x3)/2;
          cy = clipRect.yMin();
        }
        // should not happen
      }
    }
  }
  else {
//    cout << "not overlap" << endl;
  }
  return make_pair(cx, cy);
}

static std::pair<int, int> 
getCxCyBoundary(odb::Rect& rect, 
    odb::Rect& clipRect) {

  int cx = -1, cy = -1;
  // check if box is lying in four boundary 
  // overlapped on  
  // UP
  if( clipRect.xMin() <= rect.xMin() &&
      clipRect.xMax() >= rect.xMax() &&
      rect.yMin() <= clipRect.yMax() &&
      clipRect.yMax() <= rect.yMax() ) {
    cx = (rect.xMin() + rect.xMax())/2;
    cy = clipRect.yMax();
  }
  // DOWN
  else if( clipRect.xMin() <= rect.xMin() &&
      clipRect.xMax() >= rect.xMax() &&
      rect.yMin() <= clipRect.yMin() &&
      clipRect.yMin() <= rect.yMax() ) {
    cx = (rect.xMin() + rect.xMax())/2;
    cy = clipRect.yMin();
  }
  // LEFT
  else if( clipRect.yMin() <= rect.yMin() &&
      clipRect.yMax() >= rect.yMax() &&
      rect.xMin() <= clipRect.xMin() &&
      clipRect.xMin() <= rect.xMax() ) {
    cx = clipRect.xMin();
    cy = (rect.yMin() + rect.yMax())/2;
  }
  // RIGHT
  else if( clipRect.yMin() <= rect.yMin() &&
      clipRect.yMax() >= rect.yMax() &&
      rect.xMin() <= clipRect.xMax() &&
      clipRect.xMax() <= rect.xMax() ) {
    cx = clipRect.xMax();
    cy = (rect.yMin() + rect.yMax())/2;
  }
  return std::make_pair(cx,cy);
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

// with given insts.
bool Graph::init(std::set<odb::dbNet*> & nets, 
    std::set<odb::dbInst*>& insts,
    odb::Rect* clipRect,
    GraphModel gModel, 
    EdgeWeightModel eModel,
    int verbose) {

  verbose_ = verbose;

  // init vertexMap_, and vertices_
  int vertexId = 0;

  // temporary index inference hash map
  // Note that vertexMap_ will be updated after 
  // vertices_ is finalized.
  std::map<odb::dbInst*, int> instToIdx;
  for(auto& inst : insts) {
    vertices_.push_back(Vertex(inst, vertexId++, 0.0f));

    // this is safe because of "reserve"
    instToIdx[inst] = vertices_.size() -1;
  }

  if( verbose > 3 ) {
    cout << "clipRect " << clipRect->xMin() << " " << clipRect->yMin() << " ";
    cout << clipRect->xMax() << " " << clipRect->yMax() << endl;
  }

  // Sturct for save data
  struct NetInfo {
    public:
      odb::dbNet* net;
      int outVertIdx;
      int inVertOutsideClipIdx;
      vector<int> inVertIdxes;
    NetInfo() : net(nullptr), outVertIdx(-1), inVertOutsideClipIdx(-1) {};
  };


  vector<NetInfo> netInfos;

  // foreach net
  for(odb::dbNet* net : nets) {
    // insts/bTerms outpins.
    Pin outPin;

    // insts/bTerms inside of clips (in direction)
    vector<Pin> inPins;

    // insts/bTerms outside of clips (in direction)
    vector<Pin> inPinsOutsideClip;

    int outputCnt = 0;

    for(odb::dbBTerm* bTerm : net->getBTerms()) {
      odb::Rect bRect = bTerm->getBBox();
      // this is OUTPUT pins
      if( bTerm->getIoType() == odb::dbIoType::INPUT ) {
        if( isLineOverlap(bRect, *clipRect) ) {
          outPin = Pin(bTerm, false);
        }
        else {
          outPin = Pin(bTerm, true);
        }
        outputCnt += 1;
      }
      // this is INPUT pins 
      else if (bTerm->getIoType() == odb::dbIoType::OUTPUT ) {
        if( isLineOverlap(bRect, *clipRect) ) {
          inPins.push_back( Pin(bTerm, false) );
        } 
        else {
          inPinsOutsideClip.push_back( Pin( bTerm, true) );
        }
      }
    }

    for(odb::dbITerm* iTerm : net->getITerms()) {
      odb::Rect iRect = iTerm->getBBox();
      if( iTerm->getIoType() == odb::dbIoType::INPUT ) {
        if( isLineOverlap(iRect, *clipRect) ) {
          inPins.push_back( Pin(iTerm, false) );
        }
        else {
          inPinsOutsideClip.push_back( Pin( iTerm, true) );
        }
      }
      else if (iTerm->getIoType() == odb::dbIoType::OUTPUT) {
        if( isLineOverlap(iRect, *clipRect) ) {
          outPin = Pin(iTerm, false);
        }
        else {
          outPin = Pin(iTerm, true);
        }

        outputCnt += 1;
      }
    }

//    cout << net->getConstName() << endl;
//    cout << "output info" << endl;
//    outPin.print();

//    cout << "inpins info" << endl;
//    for(auto& inPin : inPins) {
//      inPin.print();
//    }

    if( verbose_ > 1 && outputCnt != 1 ) {
      cout << "WARNING: multiple outputs detected in nets: " << net->getConstName() << endl;
    }

    // initialize NetInfo to save current info
    NetInfo netInfo;
    netInfo.net = net;

    // find outpin (virtual)
    // at least one of inPins must be inside clips. 
    if( outPin.isOutside_ == true ) {
      // check the assumption
      assert( inPin.size() > 0 );

      // lets find entry point
      odb::dbWireGraph graph;
      graph.decode(net->getWire());

      // output points' center coordinates
      int cx = -1, cy = -1;
      for(odb::dbWireGraph::edge_iterator edgeIter = graph.begin_edges();
          edgeIter != graph.end_edges();
          edgeIter ++ ) {
        odb::dbWireGraph::Edge* edge = *edgeIter;

        int x1=-1, y1=-1, x3=-1, y3=-1;
        edge->source()->xy(x1, y1);
        edge->target()->xy(x3, y3);

//        cout << "[EDGE]: " << x1 << " " << y1 << " " <<x3 << " " <<y3 << " ";

        if ( x1 != x3 || y1 != y3 ) {
          auto center = getCxCyEntry(x1,y1,x3,y3, *clipRect);
          cx = center.first;
          cy = center.second;
        }
        else {
//          cout << endl;
        }

        if( cx != -1 && cy != -1) {
          break;
        }

        // corner case handling:
        // consider source/target object
        vector<odb::dbObject*> objects;
        if( edge->source()->object() ) {
          objects.push_back( edge->source()->object() );
        }
        if( edge->target()->object() ) {
          objects.push_back( edge->target()->object() );
        }

        for( auto& object : objects ) {
          odb::Rect rect;
          if( object->getObjName() == "dbITerm" ) { 
            odb::dbITerm* iTerm = odb::dbITerm::getITerm(db_->getChip()->getBlock(), object->getId());
//            printITerm(db_->getChip()->getBlock(), iTerm);
//            cout << endl;
            rect = iTerm->getBBox();
          }
          else if( object->getObjName() == "dbBTerm" ) {
            odb::dbBTerm* bTerm = odb::dbBTerm::getBTerm(db_->getChip()->getBlock(), object->getId());
//            printBTerm(db_->getChip()->getBlock(), bTerm);
//            cout << endl;
            rect = bTerm->getBBox();
          }
          auto center = getCxCyBoundary(rect, *clipRect);
          cx = center.first;
          cy = center.second;

          if( cx != -1 && cy != -1) {
            break;
          }
        }

        if( cx != -1 && cy != -1) {
          break;
        }
      } 

      if( cx == -1 || cy == -1 ){
        if( verbose_ > 1 ) {
          cout <<"ERROR: cannot find incoming edge " << endl;
        }
        return false;
      } 

      if( verbose_ > 3 ) {
        cout << "Found incoming edge point: " << cx << " " << cy << endl;
      }
      // update vertices
      vertices_.push_back( Vertex(cx, cy, vertexId++, 0.0f) );
      netInfo.outVertIdx = vertices_.size()-1;
    }  
    // the outpin is inside of clips.
    else {

      // bterm cases --> need to push
      if( outPin.type_ == Pin::Type::BTERM ) {
        vertices_.push_back( Vertex( outPin.bTerm_, vertexId++, 0.0f) );
        netInfo.outVertIdx = vertices_.size()-1; 
      }
      // iterm cases --> retrieve the pushed dbInsts.
      else if( outPin.type_ == Pin::Type::ITERM) { 
        odb::dbInst* inst = outPin.iTerm_->getInst();
        auto idPtr = instToIdx.find(inst);
        if( idPtr == instToIdx.end() ) {
          if( verbose_ > 1 ) {
            cout << "ERROR: cannot find inst in instToIdx map." << endl;
          }
          return false;
        }
        netInfo.outVertIdx = idPtr->second;
      }
    }

    if( netInfo.outVertIdx == -1 ) {
      if( verbose_ > 1 ) {
        cout << "ERROR: CANNOT FIND OUTVERT" << endl;
      }
      return false;
    }


    // find exit point (virtual)
    // only if there IS inPins outside of clips 
    if( inPinsOutsideClip.size() > 0 && outPin.isOutside_ == false ) {

      odb::dbWireGraph graph;
      graph.decode(net->getWire());

      int cx = -1, cy = -1;
      for(odb::dbWireGraph::edge_iterator edgeIter = graph.begin_edges();
          edgeIter != graph.end_edges();
          edgeIter ++ ) {
        odb::dbWireGraph::Edge* edge = *edgeIter;

        int x1=-1, y1=-1, x3=-1, y3=-1;
        edge->source()->xy(x1, y1);
        edge->target()->xy(x3, y3);

//        cout << "[EDGE]: " << x1 << " " << y1 << " " <<x3 << " " <<y3 << " ";

        if ( x1 != x3 || y1 != y3 ) {
          auto center = getCxCyExit(x1,y1,x3,y3, *clipRect);
          cx = center.first;
          cy = center.second;
        }
        else {
//          cout << endl;
        }

        if( cx != -1 && cy != -1) {
          break;
        }

        // corner case handling:
        // consider source/target object
        vector<odb::dbObject*> objects;
        if( edge->source()->object() ) {
          objects.push_back( edge->source()->object() );
        }
        if( edge->target()->object() ) {
          objects.push_back( edge->target()->object() );
        }

        for( auto& object : objects ) {
          odb::Rect rect;
          if( object->getObjName() == "dbITerm" ) { 
            odb::dbITerm* iTerm = odb::dbITerm::getITerm(db_->getChip()->getBlock(), object->getId());
//            printITerm(db_->getChip()->getBlock(), iTerm);
//            cout << endl;
            rect = iTerm->getBBox();
          }
          else if( object->getObjName() == "dbBTerm" ) {
            odb::dbBTerm* bTerm = odb::dbBTerm::getBTerm(db_->getChip()->getBlock(), object->getId());
//            printBTerm(db_->getChip()->getBlock(), bTerm);
//            cout << endl;
            rect = bTerm->getBBox();
          }
          auto center = getCxCyBoundary(rect, *clipRect);
          cx = center.first;
          cy = center.second;

          if( cx != -1 && cy != -1) {
            break;
          }
        }

        if( cx != -1 && cy != -1) {
          break;
        }
      }

      if( cx == -1 || cy == -1 ){
        if( verbose_ > 1 ) {
          cout <<"ERROR: cannot find outgoing edge " << endl;
        }
        return false;
      } 
      else {
        if( verbose_ > 1 ) {
          cout << "Found outgoing edge point: " << cx << " " << cy << endl;
        }

        vertices_.push_back( Vertex(cx, cy, vertexId++, 0.0f) ); 
        netInfo.inVertOutsideClipIdx = vertices_.size()-1;
      }
    }

    // update vertices_ with bTerm in pin case
    for(auto& inPin : inPins ) {
      if( inPin.type_ == Pin::Type::BTERM ) {
        vertices_.push_back( 
            Vertex( inPin.bTerm_, vertexId++, 0.0f) );
        netInfo.inVertIdxes.push_back( vertices_.size() -1 );
      }
      else if( inPin.type_ == Pin::Type::ITERM ) {
        auto idPtr = instToIdx.find( inPin.iTerm_->getInst() ); 
        if( idPtr == instToIdx.end() ) {
          if( verbose_ > 1 ) {
            cout << "ERROR: cannot find inst in instToIdx" << endl;
          }
          return false;
        }
        netInfo.inVertIdxes.push_back( idPtr->second ); 
      }
    }
    netInfos.push_back(netInfo);
  }
  // net end
  
  // now, all vertices_ are finalized.
  // update vertexInstMap_ and vertexBTermMap_ 
  // to use dbToGraph function
  for(int i=0; i<vertices_.size(); i++) {
    Vertex* vertex = &vertices_[i];
    if( vertex->type() == Vertex::Type::INST) {
      vertexInstMap_[vertex->inst()] = vertex;
//      cout << "Inst: " << vertex->inst() << " " << vertex << endl;
    } 
    else if( vertex->type() == Vertex::Type::IOPORT) {
      vertexBTermMap_[vertex->bTerm()] = vertex;
//      cout << "BTerm: " << vertex->bTerm() << " " << vertex << endl;
    }
    else {
//      cout <<  "VIRTUAL: " << vertex << endl;
    }
  }

  // now fill in edges as star model
  if( gModel == GraphModel::Star ) {
    for(auto& netInfo : netInfos) { 
      // retrieve outVertex 
      Vertex* outVertex = &vertices_[netInfo.outVertIdx];
//      cout << "outVertex: " << netInfo.outVertIdx << " " << outVertex << endl;

      // for inpins inside clips
      for(auto& inVertIdx : netInfo.inVertIdxes) {
        // retrieve inVertex
        Vertex* inVertex = &vertices_[inVertIdx];
        // push edge
        edges_.push_back(
           Edge(outVertex, inVertex, 1.0)); 
//        cout << "  inVertex: " << inVertIdx << " " << inVertex << endl;
      }

      // for inpins outside clips 
      // must have inPins outside of clips
      if( netInfo.inVertOutsideClipIdx != -1 ) {
        Vertex* inVertex = &vertices_[netInfo.inVertOutsideClipIdx];
        // push edge
        edges_.push_back( Edge(outVertex, inVertex, 1.0) );
      }
    }
  }
  
  if( verbose_ > 1 ) {
    cout << "TotalVertex: " << vertices_.size() << endl;
    cout << "TotalEdge: " << edges_.size() << endl;
  }
  // vertex' inEdge/outEdge update
  updateVertsFromEdges();
  
  return true;
}

// Need for BFS  search
void Graph::updateVertsFromEdges() {
  for(auto& edge : edges_) {
    Vertex* fromVert = edge.from();
    Vertex* toVert = edge.to();
    
    if( !fromVert || !toVert ) {
      cout << "ERROR: Vertex not existed!!" << endl;
      exit(1);
    }

    fromVert->addOutEdge(&edge);
    toVert->addInEdge(&edge);
  }
}

vector<int> 
Graph::bfsSearch(Vertex* vertex) {
  queue<Vertex*> vertexQ;
  vertexQ.push(vertex);

  // visitTable
  vector<int> visitTable(vertices_.size(), -1);

  // BFS search
  while( !vertexQ.empty() ) {    
    Vertex* curVertex = vertexQ.front();
    vertexQ.pop();

    // visited
    visitTable[curVertex->id()] = 1;

    // for each adjacent vertex
    const vector<Edge*> & inEdges = curVertex->inEdges();
    if( inEdges.size() > 0 ) {
      for(auto& edge : inEdges) {
        // exist and 
        // not visited before
        if( edge->from() 
            && visitTable[edge->from()->id()] == -1) {
          vertexQ.push(edge->from());
        }
      }
    }

    // for each adjacent vertex
    const vector<Edge*> & outEdges = curVertex->outEdges();
    if( outEdges.size() > 0 ) {
      for(auto& edge : outEdges) {
        // exist and
        // not visited before
        if( edge->to() 
            && visitTable[edge->to()->id()] == -1) {
          vertexQ.push(edge->to());
        }
      }
    } 
  }
  return visitTable;
} 



// 
// edgeWeight : 0.01, 0.5, ...
// numFakeEdgePerCC : numFakeEdges that connects isolated connected-components.
// 
void
Graph::connectIsolatedClusters(float fakeEdgeWeight, 
    int numFakeEdgePerCC) {
  
  vector<int> visitTable(vertices_.size(), -1);

  // cc: connected component
  vector<vector<Vertex*>> ccs;

  int i=0;
  for(auto& vertex : vertices_) {
//    cout << "id: " << vertex.id() << endl;
    // already visited then skip
    if( visitTable[vertex.id()] != -1 ) { 
      continue;
    }

    vector<int> visit = bfsSearch(&vertex);
//    cout << vertex.inst()->getConstName() << " has ";
//    for(auto& visitVert : visit) {
//      cout << visitVert->inst()->getConstName() << " ";
//    }

    // current connected components
    vector<Vertex*> cc;

    // update vertices on visitTable / cc
    for(int i=0; i<vertices_.size(); i++) {
      if( visit[i] != -1 ) {
        visitTable[i] = 1;
        cc.push_back( &vertices_[i] );
      }
    }

    ccs.push_back(cc);
    if( verbose_ > 3 ) {
      cout << "Component: " << ccs.size() << " has " << cc.size() << " instance" << endl;
      if( cc.size() == 1 ) {
        cout << "single: " << cc[0]->inst()->getConstName() << endl; 
      }
    }
  }

  // 
  // edgeTable will have
  // vertex 2D lists with 
  //
  // numFakeEdgePerCC X ccs.size()

  vector<vector<Vertex*>> fakeEdgeTable;;

  if( verbose_ > 1 ) {
    cout << "FakeEdgeWeight: " << fakeEdgeWeight << endl;
    cout << "numFakeEdgesPerCC: " << numFakeEdgePerCC << endl;
  }

  for(auto& cc : ccs) {
    int cnt = 0;
    vector<Vertex*> vList;
    for(auto& vertex: cc) {
      vList.push_back(vertex);
      cnt++;
      if( cnt >= numFakeEdgePerCC) {
        break;
      }
    }
    // not enough, then repeat
    while( cnt < numFakeEdgePerCC ) {
      vList.push_back(cc[0]);
      cnt++;
    }

    if( verbose_ > 1 ) {
      for(int i=0; i<numFakeEdgePerCC; i++) {
        cout << vList[i]->inst()->getConstName() << " " ;
      }
      cout << endl;
    }

    fakeEdgeTable.push_back(vList);
  }

  int prevSize = edges_.size();

  for(int i=0; i<fakeEdgeTable.size()-1; i++) {
    for(int j=0; j<numFakeEdgePerCC; j++) {
      edges_.push_back(Edge(fakeEdgeTable[i][j], fakeEdgeTable[i+1][j], fakeEdgeWeight)); 
    }
  }
 
  if( verbose_ > 1 ) { 
    cout << "Added Fake Edges: " << edges_.size() - prevSize << endl;
  }
}


void Graph::printEdgeList() {
  cout << "edges: " << edges_.size() << endl;
  for(auto& edge: edges_) {
    cout << edge.from()->inst()->getConstName() << " ";
    cout << edge.to()->inst()->getConstName() << " " ;
    cout << edge.weight() << endl;
  }
}

void Graph::saveFile(std::string fileName) {
  std::ofstream outFile;
  outFile.open(fileName, std::ios_base::out); 
	for(auto& edge: edges_) {
		outFile << edge.from()->name() << " ";
		outFile << edge.to()->name() << " ";
		outFile << edge.weight() << endl;
	}
	outFile.close();
}

void Graph::saveMasterFile(std::string fileName, std::map<odb::dbMaster*, int>& masterMap) {
  std::ofstream outFile;
  outFile.open(fileName, std::ios_base::out); 
	for(auto& vertex: vertices_) {
		outFile << vertex.name() << " ";
    odb::dbMaster* master = nullptr;
    if( vertex.type() == Vertex::Type::INST) { 
      master = vertex.inst()->getMaster();
      auto mmPtr = masterMap.find(master);
      if( mmPtr == masterMap.end() ) {
        cout << "ERROR: Cannot find master cell" << endl;
        exit(1);
      }
      outFile << "t" << mmPtr->second << " ";
    }
    else if (vertex.type() == Vertex::Type::IOPORT) {
      outFile << "IOPORT ";
    }
    else if (vertex.type() == Vertex::Type::VIRTUAL) {
      outFile << "VIRTUAL ";
    }

    outFile << vertex.lx() << " " << vertex.ly() << " " 
      << vertex.ux() << " " << vertex.uy() << endl;
	}
	outFile.close();
}

Vertex* Graph::dbToGraph(odb::dbInst* inst) {
  auto vertPtr = vertexInstMap_.find(inst);
  if( vertPtr == vertexInstMap_.end() ) {
    cout << "ERROR: dbToGraph cannot find inst " << endl;
    return nullptr; 
  }
  else {
    return vertPtr->second;
  }
}
Vertex* Graph::dbToGraph(odb::dbBTerm* bTerm) {
  auto vertPtr = vertexBTermMap_.find(bTerm);
  if( vertPtr == vertexBTermMap_.end() )  {
    cout << "ERROR: dbToGraph cannot find bTerm " << endl;
    return nullptr;
  }
  else {
    return vertPtr->second;
  }
}

}

