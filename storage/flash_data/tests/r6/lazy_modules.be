# R6: on-demand native module loading / lazy proxy compatibility.
# Run on hardware after flashing R6 firmware:
#   berry profile
#   berry run /sdcard/tests/r6/lazy_modules.be   (or /flash/...)
#   berry profile
#
# Expected: prints R6 PASS lines; no crash/import error. Compare profile before/after:
# modules should load on demand when used; scripts not touching a module avoid it.

import string

print("R6 lazy module probe")

# Berry `import` path (on-demand, bypasses lazy proxy global).
import fs
var tmp = "/sdcard/r6_lazy.tmp"
var w = fs.write_text(tmp, "r6")
if w != true
    tmp = "/flash/r6_lazy.tmp"
    w = fs.write_text(tmp, "r6")
end
print("R6 import fs write", w)
var rt = fs.read_text(tmp)
print("R6 import fs read", rt["ok"], rt["text"])

# cos_import path (used by TS-compiled COS apps/scripts).
var __cos_net = cos_import("@cos/net")
var net_mod = __cos_net.find("default")
if net_mod == nil
    print("R6 FAIL: cos_import @cos/net")
else
    var base = net_mod.base_url()
    print("R6 cos_import net base_url type", type(base))
end

# Bare-global compatibility via R6 lazy proxies (console/fs/net/ui).
# `ui` proxy should load real module on first attribute access without requiring
# explicit import in legacy scripts. We only read constants (no display mutation).
print("R6 bare ui.KEY_ENTER", ui.KEY_ENTER)
print("R6 bare console.font_size", console.font_size())

# json remains on-demand (R1): explicit import is the portable Berry form.
import json
var dumped = json.dump({"r6": true, "n": 1})
print("R6 import json dump", type(dumped), size(dumped) > 0)

print("R6 OK")