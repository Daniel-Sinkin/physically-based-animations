#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

doxygen docs/Doxyfile

navtree_js="docs/technical/api/navtree.js"
if [[ -f "${navtree_js}" ]]; then
  perl -0pi -e 's/const gotoAnchor = function\(anchor,aname\) \{/const gotoAnchor = function(anchor,aname,instantJump=false) {/' "${navtree_js}"
  perl -0pi -e 's/docContent\.animate\(\{\n        scrollTop: pos \+ dcScrTop - dcOffset\n      \},Math\.max\(50,Math\.min\(500,dist\)\),function\(\) \{/docContent.stop(true,true).animate({\n        scrollTop: pos + dcScrTop - dcOffset\n      },instantJump ? 0 : Math.max(50,Math.min(500,dist)),function() {/s' "${navtree_js}"
  perl -0pi -e 's/\$\("\.page-outline a\[href\]:not\(\.noscroll\)"\)\.click\(function\(e\) \{\n      e\.preventDefault\(\);\n      const aname = \$\(this\)\.attr\("href"\);\n      gotoAnchor\(\$\(aname\),aname\);\n    \}\);/\$("\.page-outline a[href]:not(.noscroll)").click(function(e) {\n      e.preventDefault();\n      const aname = \$(this).attr("href");\n      gotoAnchor(\$(aname),aname,true);\n    });/s' "${navtree_js}"
fi
