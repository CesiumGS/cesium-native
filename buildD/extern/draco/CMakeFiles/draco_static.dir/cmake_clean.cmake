file(REMOVE_RECURSE
  "libdracod.a"
  "libdracod.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/draco_static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
