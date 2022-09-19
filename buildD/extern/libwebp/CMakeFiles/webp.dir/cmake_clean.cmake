file(REMOVE_RECURSE
  "libwebpd.a"
  "libwebpd.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C)
  include(CMakeFiles/webp.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
