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
set_db(odb::dbDatabase* db) 
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->setDb(db);
}

void
init()
{
  GraphExtractor* graphExt = getGraphExtractor();
  graphExt->init();
}

void
set_graph_extract_verbose_level_cmd(int verbose); 

void
set_graph_model_cmd(const char* model);

void
set_edge_weight_model_cmd(const char* model);

void
set_visit_depth_cmd(int depth); 

void
set_graph_extract_save_file_name_cmd(const char* file);

void
set_graph_extract_save_master_file_name_cmd(const char* file);

void
set_graph_extract_net_threshold_cmd(int threshold);

void
set_graph_extract_fake_edge_weight(float weight);

void
set_graph_extract_num_fake_edges_per_cc(int num);

void 
graph_extract_init_cmd();

bool
graph_extract_cmd(int lx, int ly, int ux, int uy);

void
graph_extract_clear_cmd();


%}
