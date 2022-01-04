#ifndef __GRAPH__EXTRACTOR__
#define __GRAPH__EXTRACTOR__

#include <iostream>
#include <vector>
#include <map>

namespace odb {
class dbDatabase;
class dbInst;
class dbMaster;
class dbITerm;
}

namespace sta {
class dbSta;
}

namespace GraphExtract {

enum GraphModel {
  Star, Clique, Hybrid
};

enum EdgeWeightModel {
  A, B, C, D, E
};

class GraphExtractor {
  public:
    void setDb(odb::dbDatabase* db);
    void setSta(sta::dbSta* sta);
    void init();
    void clear();
    void clearRTrees();
    bool extract(int lx, int ly, int ux, int uy);
    void setSaveFileName (const char* fileName);
    void setSaveMasterFileName (const char* fileName);

    void setGraphModel(const char* graphModel);
    void setEdgeWeightModel(const char* edgeWeightModel);
    void setVisitDepth(int visitDepth);
    void setNetThreshold(int threshold);
    void setFakeEdgeWeight(float weight);
    void setNumFakeEdgesPerCC(int num);
    void setVerbose(int verbose);

    GraphModel getGraphModel() { return graphModel_; }
    EdgeWeightModel getEdgeWeightModel() { return edgeWeightModel_; }

    GraphExtractor();
    ~GraphExtractor();

  private:
    odb::dbDatabase* db_;
    sta::dbSta* sta_;
    void* rTree_;
    void* wireRTree_;
    GraphModel graphModel_;
    EdgeWeightModel edgeWeightModel_;
    std::string fileName_;
    std::string masterFileName_;
    // anonym master cell type
    std::map<odb::dbMaster*, int> masterMap_;
    int visitDepth_;
    int netThreshold_;
    float fakeEdgeWeight_;
    int numFakeEdgesPerCC_;
    int verbose_;
};

}

#endif
