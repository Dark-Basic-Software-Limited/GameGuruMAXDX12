# VRAM audit categoriser. Buckets chosen to answer "which knob would move this".
# NOTE texTreeMap / texGrassMap are terrain GLOBAL MAPS (fixed floor), not tree/grass content —
# do not let the name fool the regex into double counting them as content.
NR<=2 { next }
{
  bytes=$2; misc=$11; usage=$12; name="";
  for(i=14;i<=NF;i++) name=name" "$i;
  gsub(/^ |"/,"",name);
  c="7 MISC";
  if      (misc=="0x200")                                                      c="1 FLOOR svt pool";
  else if (name ~ /VirtualTexture::/)                                          c="1 FLOOR svt bookkeeping";
  else if (name=="tex")                                                        c="1 FLOOR terrain src arrays";
  else if (name ~ /ChunkData::blendmap|^wetmap$|ChunkData::heightmap/)         c="1 FLOOR terrain chunk maps";
  else if (name ~ /^texMaterialMap$|^texGrassMap$|^texTreeMap$|^texLOD/)       c="1 FLOOR terrain global maps";
  else if (name ~ /shadowMapAtlas/)                                            c="1 FLOOR shadow atlas";
  else if (name ~ /^rt|depthBuffer|MSAA|_render$|rtFinal|luminance|bloom|ssao|Tonemap|Sharpen|Temporal|FSR|dof|motionblur/) c="1 FLOOR rendertargets";
  else if (name=="" && $1=="B")                                                c="1 FLOOR small mesh bufs";
  else if (name ~ /GPUSubAllocator/)                                           c="2 MESH POOL";
  else if (name ~ /HairParticle|hair/)                                         c="3 GRASS";
  else if (name ~ /treebank/)                                                  c="4 TREES";
  else if (name ~ /skybank|envMap|EnvMap|probe|lowres|aerial|transmittance|multiscatter/) c="6 SKY+ENV";
  else if (usage!="0")                                                         c="7 MISC upload/readback";
  else if (name ~ /streamoutBuffer/)                                           c="7 MISC skinned streamout";
  else if (name ~ /entitybank|levelbank|gamecore|imagebank|editors|databank|charactercreator|\.dds|\.png|\.jpg|Files/) c="5 CONTENT textures";
  t[c]+=bytes; n[c]++; all+=bytes;
}
END{ for(k in t) printf "%s|%.1f|%d\n", k, t[k]/1048576, n[k]; printf "ZZ CENSUS TOTAL|%.1f|%d\n", all/1048576, NR-2 }
