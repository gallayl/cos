class Clear
    def run(_args)
        action("clear")
        return 0
    end
end

var __cos_app = Clear()
def main(args)
    var __cos_r = __cos_app.run(args)
    if __cos_r == nil
        return 0
    end
    return __cos_r
end
