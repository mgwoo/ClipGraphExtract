# ClipGraphExtract (Machine Learning Tutorial)
- An example repo for extracting a clip from given coordinates..
- The base sources are copied from [OpenROAD](https://github.com/The-OpenROAD-Project/OpenROAD) repo, commit: [50f0e3](https://github.com/The-OpenROAD-Project/OpenROAD/commit/50f0e32411c12d71438481b8b353a03b45537baa). 
- The developer guide of the OpenROAD project is available in [DeveloperGuide.md](https://github.com/mgwoo/ClipGraphExtract/blob/MLtutorial/docs/contrib/DeveloperGuide.md).
- Note that this version registers Python modules (GraphExtractor_py) using SWIG.

## ClipGraphExtract Flow
| <img src="/docs/misc/clique-star-v2.png" width=600px> |
|:--:|
| Clique and star net decomposition example |
- In OpenROAD app, read_lef and read_def command will populate the OpenDB's data structure.
- Using OpenDB's C++ API, save all instances' bbox to Boost/RTree structure. 
- Send a region query to RTree to extract related instances using the clips' coordinates.
- Generate clip graph's clique/star net models as text file (e.g. edges list) for graph neural network models.


## Implementation Details
### OpenDB C++ API
- All the OpenDB reference is in the OpenDB's public header [(odb/include/opendb/db.h)](https://github.com/The-OpenROAD-Project/OpenDB/blob/ebbf56ee8ddb08f9a8da5febafe37691731f2932/include/opendb/db.h). Please check this header file first to understand how the structures are designed.

## How to build binary?
- Please follow the build manual in [OpenROAD](https://github.com/The-OpenROAD-Project/OpenROAD)

      $ mkdir build
      $ cd build
      $ cmake ..
      $ make
      
## Example Python API usages
- [generate_clips.py](https://gist.github.com/mgwoo/5612863f1cde8346ffbed488d1a706bf)
- Command:
                
      $ openroad -python generate_clips.py

## File Structure
-  include/openroad/  
    - public headers location of OpenROAD.

- src/ 
    - OpenROAD source code files. The tools source code should be located here. 
  
- src/GraphExtract/src/  
    - source code location of ClipGraphExtractor.
  
- src/GraphExtract/include/graphext/  
    - public headers location of ClipGraphExtractor.
  
      
## License
- BSD-3-Clause license. 
- Code found under the subdirectory (e.g., src/OpenSTA) have individual copyright and license declarations at each folder.
