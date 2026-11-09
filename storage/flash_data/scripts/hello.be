class Hello
    def run(_args)
        log("Hello from COS Berry!")
        return 0
    end
end

var __cos_app = Hello()
def main(args)
    var __cos_r = __cos_app.run(args)
    if __cos_r == nil
        return 0
    end
    return __cos_r
end
if !(_cos_cmd_exec == true)
    main([])
end
