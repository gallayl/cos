def main(args)
    var index = 0
    while index < args.size()
        print(str(index + 1) + ": " + args[index])
        index += 1
    end
    return 0
end
