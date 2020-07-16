%module ClipGraphExtractor

%{
#include "openroad/OpenRoad.hh"
#include "clip_graph_ext/clipGraphExtractor.h"

namespace ord {
ClipGraphExtract::ClipGraphExtractor*
getClipGraphExtractor(); 
odb::dbDatabase*
getDb();
sta::dbSta*
getSta();
}


namespace ClipGraphExtract { 
enum GraphModel;
enum EdgeWeightModel;
}

using ord::getClipGraphExtractor;
using ord::getDb;
using ord::getSta;
using ClipGraphExtract::ClipGraphExtractor;
using ClipGraphExtract::GraphModel;
using ClipGraphExtract::EdgeWeightModel;

%}

%inline %{

void
set_graph_model_cmd(const char* model) 
{
  ClipGraphExtractor* graphExt = getClipGraphExtractor();
  graphExt->setGraphModel(model);
}

void
set_edge_weight_model_cmd(const char* model)
{
  ClipGraphExtractor* graphExt = getClipGraphExtractor();
  graphExt->setEdgeWeightModel(model);
}

void
set_graph_extract_save_file_name_cmd(const char* file)
{
  ClipGraphExtractor* graphExt = getClipGraphExtractor();
  graphExt->setSaveFileName(file);
}

void 
graph_extract_init_cmd()
{
  ClipGraphExtractor* graphExt = getClipGraphExtractor();
  graphExt->setDb(getDb());
  graphExt->setSta(getSta());
  graphExt->init();
}

void
graph_extract_cmd(int lx, int ly, int ux, int uy) 
{
  ClipGraphExtractor* graphExt = getClipGraphExtractor();
  graphExt->extract(lx, ly, ux, uy);
}


void
graph_extract_clear_cmd() 
{
  ClipGraphExtractor* graphExt = getClipGraphExtractor();
  graphExt->clear();
}

%}
