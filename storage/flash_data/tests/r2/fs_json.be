# R2/P7: filesystem JSON read/write regression + lazy JSON profiling checkpoint.
import fs

var path = "/sdcard/r2_fs_json.tmp"
var payload = {"ok": true, "n": 42, "s": "r2"}
var probe = fs.write_json(path, payload)
if probe != true
    path = "/flash/r2_fs_json.tmp"
    probe = fs.write_json(path, payload)
end
var loaded = fs.read_json(path)
print("fs_json write", probe)
print("fs_json ok", loaded["ok"])
if loaded["ok"]
    var value = loaded["value"]
    print("fs_json value", value["ok"], value["n"], value["s"])
else
    print("fs_json error", loaded["error"])
end
print("next: berry profile")