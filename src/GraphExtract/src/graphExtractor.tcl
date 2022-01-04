
sta::define_cmd_args "graph_extract" {
  [-neighbor_level level]\
  [-graph_model star/clique]\
  [-edge_weight_model weight]\
  [-net_threshold threshold]\
  [-out_file fileName]
}

#sta::define_cmd_args "graph_extract" {
#  [-global_config global_config_file]\
#  [-local_config local_config_file]}

proc graph_extract { args } {
  sta::parse_key_args "graph_extract" args \
    keys {-neighbor_level -graph_model -edge_weight_model \
      -net_threshold -fake_edge_weight -num_fake_edges_per_cc -out_file -out_master_file -verbose} flags {}

  # default model is star
  set graph_model "star"
  if { [info exists keys(-graph_model)] } {
    set graph_model $keys(-graph_model)
    GraphExtractor::set_graph_model_cmd $graph_model
  }

  # default weight_model is xxx
  set edge_weight_model "a"
  if { [info exists keys(-edge_weight_model)] } {
    set edge_weight_model $keys(-edge_weight_model)
    GraphExtractor::set_edge_weight_model_cmd $edge_weight_model
  }
  set visit_depth 1
  if { [info exists keys(-neighbor_level)] } {
    set visit_depth $keys(-neighbor_level)
    GraphExtractor::set_visit_depth_cmd $visit_depth
  }

  if { [info exists keys(-net_threshold)] } {
    set threshold $keys(-net_threshold)
    GraphExtractor::set_graph_extract_net_threshold_cmd $threshold
  }

  if { ![info exists keys(-out_file)] } {
    puts "ERROR: -out_file must be used"
    return
  } else {
    set file_name $keys(-out_file)
    GraphExtractor::set_graph_extract_save_file_name_cmd $file_name
  }

  if { ![info exists keys(-out_master_file)] } {
    puts "WARNING: -out_master_file is net set"
  } else {
    set master_name $keys(-out_master_file)
    GraphExtractor::set_graph_extract_save_master_file_name_cmd $master_name
  }

  if { [info exists keys(-fake_edge_weight)] } {
    set fake_edge_weight $keys(-fake_edge_weight)
    GraphExtractor::set_graph_extract_fake_edge_weight $fake_edge_weight
  }

  if { [info exists keys(-num_fake_edges_per_cc)] } {
    set num_fake_edges_per_cc $keys(-num_fake_edges_per_cc)
    GraphExtractor::set_graph_extract_num_fake_edges_per_cc $num_fake_edges_per_cc
  }

  if { [info exists keys(-verbose)] } {
    set verbose $keys(-verbose)
    GraphExtractor::set_graph_extract_verbose_level_cmd $verbose
  }

  if { [ord::db_has_rows] } {
    GraphExtractor::graph_extract_init_cmd
    set clist [split $args]
    set result [GraphExtractor::graph_extract_cmd [lindex $clist 0] [lindex $clist 1] [lindex $clist 2] [lindex $clist 3]]
    
    # graph_extract_cmd args[0] args[1] args[2] args[3]
    GraphExtractor::graph_extract_clear_cmd
    return $result
  }
}

