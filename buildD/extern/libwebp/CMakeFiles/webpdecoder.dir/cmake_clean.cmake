file(REMOVE_RECURSE
  "libwebpdecoderd.a"
  "libwebpdecoderd.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/webpdecoder.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
