# R2/P7: filesystem text read/write regression + profiling checkpoint.
import fs

def run(args)
    var path = "/sdcard/r2_fs_text.tmp"
    var probe = fs.write_text(path, "r2-text")
    if probe == true
        var text = fs.read_text(path)
        print("fs_text sd ok", text["ok"], text["text"])
    else
        path = "/flash/r2_fs_text.tmp"
        var written = fs.write_text(path, "r2-text")
        var text = fs.read_text(path)
        print("fs_text flash", written, text["ok"], text["text"])
    end
    print("next: berry profile")
    return 0
end
