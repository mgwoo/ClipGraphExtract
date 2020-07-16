set design aes_cipher_top
read_lef NangateOpenCellLibrary.lef
read_def $design.def

read_liberty NangateOpenCellLibrary_typical.lib
read_sdc $design.sdc

graph_extract -out_file extract_clique_a.txt -graph_model clique -edge_weight_model a 500000 500000 550000 550000 
graph_extract -out_file extract_clique_b.txt -graph_model clique -edge_weight_model b 500000 500000 550000 550000 
graph_extract -out_file extract_clique_c.txt -graph_model clique -edge_weight_model c 500000 500000 550000 550000 
graph_extract -out_file extract_clique_d.txt -graph_model clique -edge_weight_model d 500000 500000 550000 550000 
graph_extract -out_file extract_clique_e.txt -graph_model clique -edge_weight_model e 500000 500000 550000 550000 
puts "clique model end"

graph_extract -out_file extract_star_a.txt -graph_model star -edge_weight_model a 500000 500000 550000 550000 
graph_extract -out_file extract_star_b.txt -graph_model star -edge_weight_model b 500000 500000 550000 550000 
graph_extract -out_file extract_star_c.txt -graph_model star -edge_weight_model c 500000 500000 550000 550000 
graph_extract -out_file extract_star_d.txt -graph_model star -edge_weight_model d 500000 500000 550000 550000 
graph_extract -out_file extract_star_e.txt -graph_model star -edge_weight_model e 500000 500000 550000 550000 
puts "star model end"

exit

