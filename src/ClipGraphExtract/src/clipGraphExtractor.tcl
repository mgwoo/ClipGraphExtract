
sta::define_cmd_args "graph_extract" {
  [-graph_model star/clique]\
  [-edge_weight_model weight]\
  [-out_file fileName]
}

proc graph_extract { args } {
  sta::parse_key_args "graph_extract" args \
    keys {-graph_model -edge_weight_model -out_file} flags {}

  # default model is star
  set graph_model "star"
  if { [info exists keys(-graph_model)] } {
    set graph_model $keys(-graph_model)
    set_graph_model_cmd $graph_model
  }

  # default weight_model is xxx
  set edge_weight_model "a"
  if { [info exists keys(-edge_weight_model)] } {
    set edge_weight_model $keys(-edge_weight_model)
    set_edge_weight_model_cmd $edge_weight_model
  }

  if { ![info exists keys(-out_file)] } {
    puts "ERROR: -out_file must be used"
    return
  } else {
    set file_name $keys(-out_file)
    set_graph_extract_save_file_name_cmd $file_name
  }

  if { [ord::db_has_rows] } {
    graph_extract_init_cmd 
    set clist [split $args]
    graph_extract_cmd [lindex $clist 0] [lindex $clist 1] [lindex $clist 2] [lindex $clist 3]
    
    # graph_extract_cmd args[0] args[1] args[2] args[3]
    graph_extract_clear_cmd
  }
}
