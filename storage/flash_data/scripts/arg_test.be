class ArgTest
    def run(args)
        var index = 0
        while index < size(args)
            print(str(index + 1) + ": " + args[index])
            index += 1
        end
        return 0
    end
end

var __cos_app = ArgTest()
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
