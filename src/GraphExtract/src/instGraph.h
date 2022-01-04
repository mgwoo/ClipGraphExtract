#ifndef __INST_GRAPH__
#define __INST_GRAPH__

#include <graphext/GraphExtractor.h>
#include <map>
#include <set>

namespace odb {
  class dbInst;
  class dbDatabase;
  class Rect;
  class dbITerm;
  class dbBTerm;
}

// only consider two-way connections
namespace GraphExtract {

struct Pin {
  public:
  enum Type {
    ITERM,
    BTERM, 
    TYPENONE
  };

  enum IO {
    INPUT,
    OUTPUT,
    IONONE
  };

  Type type_;
  IO io_;
  odb::dbITerm* iTerm_;
  odb::dbBTerm* bTerm_;
  bool isOutside_;

  Pin();
  Pin(odb::dbITerm* iTerm, bool isOutside);
  Pin(odb::dbBTerm* bTerm, bool isOutside);

  void setIo(IO io);
  void setType(Type type);
  void setIsOutside(bool isOutside);

  void print();
};

class Edge;
// vertex is equal to dbInst
class Vertex {
public:
  enum Type {
    INST, 
    IOPORT,
    VIRTUAL,
    NONE
  };

  Vertex();

  // Vertex could be 
  // 1. dbInst
  // 2. dbBTerm
  // 3. virtual boundary points (entry/exit)
  Vertex(odb::dbInst* inst,
      int id,  
      float weight);

  Vertex(odb::dbBTerm* bTerm,
      int id,
      float weight);

  Vertex(int cx, int cy,
      int id,
      float weight);

  odb::dbInst* inst() const;
  odb::dbBTerm* bTerm() const;
  const std::vector<Edge*> & inEdges() const; 
  const std::vector<Edge*> & outEdges() const; 

  float weight() const; 
  void setWeight(float weight);

  void addInEdge(Edge* edge);
  void addOutEdge(Edge* edge);

  void setLocation(int lx, int ly,
      int ux, int uy);

  void setId(int id);
  int id() const;

  Type type() const;
  std::string name();

  int lx() const;
  int ly() const;
  int ux() const;
  int uy() const;

private:
  odb::dbInst* inst_;
  odb::dbBTerm* bTerm_;
  std::vector<Edge*> inEdges_;
  std::vector<Edge*> outEdges_;
  Type type_;
  float weight_;
  int id_;
  int lx_, ly_, ux_, uy_;
};

inline odb::dbInst* Vertex::inst() const {
  return inst_;
}

inline odb::dbBTerm* Vertex::bTerm() const {
  return bTerm_;
}

inline const std::vector<Edge*> & Vertex::inEdges() const {
  return inEdges_;
}

inline const std::vector<Edge*> & Vertex::outEdges() const {
  return outEdges_;
}

inline float Vertex::weight() const {
  return weight_;
}

inline void Vertex::setId(int id) {
  id_ = id;
}

inline int Vertex::id() const {
  return id_;
}

inline Vertex::Type Vertex::type() const {
  return type_;
}

inline int Vertex::lx() const {
  return lx_;
}

inline int Vertex::ly() const {
  return ly_;
}

inline int Vertex::ux() const {
  return ux_;
}

inline int Vertex::uy() const {
  return uy_;
}

// edge is inst1-inst2 connections
class Edge {
public:
  Edge();
  Edge(Vertex* from, Vertex* to, float weight);

  Vertex* from() const;
  Vertex* to() const;
  float weight() const;

  void setFrom(Vertex* inst);
  void setTo(Vertex* inst);
  void setWeight(float weight);

private:
  Vertex* from_;
  Vertex* to_;
  float weight_;
};

inline Vertex* Edge::from() const {
  return from_;
}

inline Vertex* Edge::to() const {
  return to_;
}

inline float Edge::weight() const {
  return weight_;
}

class Graph {
public:
  Graph();
  ~Graph();

  void saveFile(std::string fileName);
  void saveMasterFile(std::string fileName, std::map<odb::dbMaster*, int>& masterMap);
  void setDb(odb::dbDatabase* db);

  bool init(std::set<odb::dbNet*> & nets, 
      std::set<odb::dbInst*> & insts,
      odb::Rect* clipRect,
      GraphModel gModel,
      EdgeWeightModel eModel,
      int verbose = 0);
  void printEdgeList();
  
  Vertex* dbToGraph(odb::dbInst* inst);
  Vertex* dbToGraph(odb::dbBTerm* bTerm);

  // returns visit table
  std::vector<int> bfsSearch(Vertex* vertex);

  void connectIsolatedClusters(float edgeWeight,
      int numFakeEdgePerCluster);

private:
  odb::dbDatabase* db_;
  std::vector<Vertex> vertices_;
  std::vector<Edge> edges_;
  
  // retrieve 
  std::map<odb::dbInst*, Vertex*> vertexInstMap_;
  std::map<odb::dbBTerm*, Vertex*> vertexBTermMap_;

  int verbose_;

  void updateVertsFromEdges();
};

}

#endif
