%{
#include "ord/OpenRoad.hh"
#include "graphext/GraphExtractor.h"

namespace ord {
OpenRoad*
getOpenRoad();

GraphExtract::GraphExtractor*
getGraphExtractor(); 
}


namespace GraphExtract { 
enum GraphModel;
enum EdgeWeightModel;
}

using ord::getOpenRoad;
using ord::getGraphExtractor;
using GraphExtract::GraphExtractor;
using GraphExtract::GraphModel;
using GraphExtract::EdgeWeightModel;

%}

// %include "../../Exception.i"

%inline %{

void
set_graph_extract_verbose_level_cmd(int verbose) 
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setVerbose(verbose);
}

void
set_graph_model_cmd(const char* model) 
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setGraphModel(model);
}

void
set_edge_weight_model_cmd(const char* model)
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setEdgeWeightModel(model);
}

void
set_visit_depth_cmd(int depth)
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setVisitDepth(depth);
}

void
set_graph_extract_save_file_name_cmd(const char* file)
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setSaveFileName(file);
}

void
set_graph_extract_save_master_file_name_cmd(const char* file)
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setSaveMasterFileName(file);
}


void
set_graph_extract_net_threshold_cmd(int threshold)
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setNetThreshold(threshold);
}

void
set_graph_extract_fake_edge_weight(float weight)
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setFakeEdgeWeight(weight);
} 

void
set_graph_extract_num_fake_edges_per_cc(int num)
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setNumFakeEdgesPerCC(num);
} 

void 
graph_extract_init_cmd()
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setDb(getOpenRoad()->getDb());
  graphExt->setSta(getOpenRoad()->getSta());
  graphExt->init();
}

bool
graph_extract_cmd(int lx, int ly, int ux, int uy) 
{
  GraphExtractor* graphExt = getGraphExtractor();
  return graphExt->extract(lx, ly, ux, uy);
}


void
graph_extract_clear_cmd() 
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->clear();
}

%}
