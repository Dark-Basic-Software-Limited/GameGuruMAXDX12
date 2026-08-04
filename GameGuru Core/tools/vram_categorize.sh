#!/bin/bash
# Categorize a VRAM census dump into line items. Usage: vram_categorize.sh <census file>
F="$1"
echo "### $(basename "$F")"
head -1 "$F" | tr ' ' '\n' | grep -E "^(census_bytes|census_default_heap|d3d12ma_blocks|driver_usage)=" \
  | awk -F= '{printf "  %-20s %8.1f MB\n", $1, $2/1048576}'
echo "  ---------------------------------------------------------------"
awk 'NR>2 {
  name=""; for(i=14;i<=NF;i++) name=name (i>14?" ":"") $i; gsub(/"/,"",name);
  kind=$1; bytes=$2; w=$3; h=$4; arr=$7; fmt=$9;
  cat="Z other";
  if (name ~ /HairParticleSystem/)                         cat="A grass/hair strand buffers (wiHairParticle)";
  else if (name == "renderTex" && w>=8192)                 cat="B GGTerrain VT physical pages (custom path)";  # real VT pages are 9520 wide; 4096x64 are GPU-particle noise strips
  else if (name == "tex" && w>=1024 && arr>=8)             cat="C GGTerrain/grass/tree source texture ARRAYS";
  else if (name == "GPUSubAllocator")                      cat="D mesh-data suballocator blocks";
  else if (kind=="B" && $11=="0x200" && bytes>100000000)   cat="E engine terrain SVT tile pool (sparse atlas)";
  else if (name ~ /shadowMapAtlas_Transparent/)            cat="F shadow atlas - TRANSPARENT (RGBA16F)";
  else if (name ~ /shadowMapAtlas/)                        cat="G shadow atlas - depth (D32)";
  else if (name ~ /ChunkData::blendmap/)                   cat="H terrain chunk blendmaps";
  else if (name ~ /wetmap/)                                cat="I terrain chunk wetmaps";
  else if (name ~ /ChunkData::heightmap/)                  cat="J terrain chunk heightmaps";
  else if (name ~ /VirtualTexture/)                        cat="K terrain SVT bookkeeping buffers";
  else if (name ~ /texMaterialMap|texGrassMap|texTreeMap|texPageTable|texLOD|texMask/) cat="L GGTerrain map/pagetable textures";
  else if (name ~ /streamoutBuffer/)                       cat="M skinned mesh streamout";
  else if (name ~ /dds|png|jpg|DDS|PNG|tga/)               cat="N content textures (entities/terrain source files)";
  else if (name ~ /^rt|depthBuffer|debugUAV|lowres|Postprocess/) cat="O render targets + postFX chain";
  else if (name ~ /CopyAllocator|frame_allocator|UploadBuffer|upload/) cat="P upload/staging buffers";
  else if (name ~ /MeshComponent|Scene::|geometryBuffer|instanceBuffer/) cat="Q scene/mesh buffers (named)";
  else if (name ~ /EmittedParticle|GPUParticle/)           cat="R particles";
  else if (name ~ /Ocean|displacementMap|gradientMap/)     cat="S ocean";
  else if (name ~ /cloud|Cloud|weatherMap|shapeNoise|erosion/) cat="T volumetric clouds/sky";
  else if (name == "")                                     cat="U unnamed (mostly small mesh/BVH allocs)";
  s[cat]+=bytes; c[cat]++;
  tot+=bytes;
} END {
  for (k in s) printf "  %9.1f MB  %5.1f%%  x%-5d %s\n", s[k]/1048576, 100*s[k]/tot, c[k], substr(k,3);
}' "$F" | sort -rn
