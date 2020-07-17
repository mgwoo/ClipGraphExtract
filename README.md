# ClipGraphExtract
- An example repo for adding the tool in DAC 2020 tutorial 8.
- The ClipGraphExtract is an example of using the OpenDB-C++-API and porting the projects to the top-level app.
- The base sources are copied from [OpenROAD](https://github.com/The-OpenROAD-Project/OpenROAD) repo, commit: [7156dc](https://github.com/The-OpenROAD-Project/OpenROAD/commit/7156dc41b0be75e9090b456103a2a1510913a4d2)
- Please read the [doc/OpenRoadArch.md](https://github.com/The-OpenROAD-Project/OpenROAD/blob/master/doc/OpenRoadArch.md) to understand the requirement. The ClipGraphExtract is an  example that follows the OpenRoadArch.md.

# ClipGraphExtract Flow
| <img src="/doc/clique-star.png" width=600px> |
|:--:|
| Clique and star net decomposition example |
- Takes LEF/DEF using OpenDB
- Save all instances' bbox to Boost/RTree structure. 
- Send region query to RTree to get related instances using the clips' coordinates.
- Generate clip graph's clique/star net models as text file (e.g. edges list) for graph neural network models.

# File Structure
-  include/openroad/  
    - public headers location of OpenROAD.

- src/ 
    - OpenROAD source code files. The tools source code should be located in here. 
  
- src/ClipGraphExtractor/src/  
    - source code location of ClipGraphExtractor.
  
- src/ClipGraphExtractor/include/clip_graph_ext/  
    - public headers location of ClipGraphExtractor.
  

# Implementation Details
- in src/
Used OpenDB C++ API to 



# License
- BSD-3-Clause license. 
- Code found under the sub directory (e.g., src/OpenSTA) have individual copyright and license declarations at each folder.
