var started = console.read_line("Input: ", def (value)
    if value == nil
        log("INPUT_CANCELLED")
    else
        log("INPUT_VALUE:" + str(value))
    end
end)
if started == true
    log("INPUT_STARTED")
else
    log("INPUT_START_ERROR:" + str(started))
end
def run(args)
    return 0
end
