#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

for tool in doxygen rg perl; do
  if ! command -v "${tool}" >/dev/null 2>&1; then
    echo "Missing required tool: ${tool}" >&2
    exit 1
  fi
done

doxygen docs/Doxyfile

navtree_js="docs/technical/api/navtree.js"
if [[ ! -f "${navtree_js}" ]]; then
  echo "Expected generated file missing: ${navtree_js}" >&2
  exit 1
fi

if ! rg -q 'document\.documentElement\.classList\.add\("doxy-loading"\)' "${navtree_js}"; then
  perl -0pi -e 's/function initNavTree\(/document.documentElement.classList.add("doxy-loading");\nwindow.addEventListener("load", function() { document.documentElement.classList.remove("doxy-loading"); }, { once: true });\n\nfunction initNavTree(/' "${navtree_js}"
fi
perl -0pi -e 's/\$\(document\)\.ready\(function\(\) \{ initPageToc\(\); initResizable\(\); \}\);/\$(document).ready(function() { initPageToc(); initResizable(); document.documentElement.classList.remove("doxy-loading"); });/' "${navtree_js}"
perl -0pi -e 's/const gotoAnchor = function\(anchor,aname\) \{/const gotoAnchor = function(anchor,aname,instantJump=false) {/' "${navtree_js}"
perl -0pi -e 's/docContent\.animate\(\{\n        scrollTop: pos \+ dcScrTop - dcOffset\n      \},Math\.max\(50,Math\.min\(500,dist\)\),function\(\) \{/docContent.stop(true,true).animate({\n        scrollTop: pos + dcScrTop - dcOffset\n      },instantJump ? 0 : Math.max(50,Math.min(500,dist)),function() {/s' "${navtree_js}"
perl -0pi -e 's/\$\("\.page-outline a\[href\]:not\(\.noscroll\)"\)\.click\(function\(e\) \{\n      e\.preventDefault\(\);\n      const aname = \$\(this\)\.attr\("href"\);\n      gotoAnchor\(\$\(aname\),aname\);\n    \}\);/\$("\.page-outline a[href]:not(.noscroll)").click(function(e) {\n      e.preventDefault();\n      const aname = \$(this).attr("href");\n      gotoAnchor(\$(aname),aname,true);\n    });/s' "${navtree_js}"

for pattern in \
  'document\.documentElement\.classList\.add\("doxy-loading"\)' \
  'const gotoAnchor = function\(anchor,aname,instantJump=false\)' \
  'instantJump \? 0' \
  'gotoAnchor\(\$\(aname\),aname,true\)'; do
  if ! rg -q "${pattern}" "${navtree_js}"; then
    echo "Expected navtree patch missing: ${pattern}" >&2
    exit 1
  fi
done
