#!/bin/zsh
source ~/.zshrc
{ ds.clipper src/pba/physics; ds.clipper src/pba/engine; ds.clipper src/pba/scene; ds.clipper src/pba/gfx; } | pbcopy
