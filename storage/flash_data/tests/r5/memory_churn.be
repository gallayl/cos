# R5: Berry memory churn probe (run on hardware).
# Usage:
#   berry profile
#   berry run /flash/tests/r5/memory_churn.be
#   berry profile
#   berry gc
# Repeat and compare heap_free/heap_min/internal_largest/berry_gc managed bytes.
# For app restart-loop regression, use the hardware guide:
#   berry run /flash/apps/tap_dash.be  (or breakout)
#   berry close
#   berry profile
# Repeat 10+ times and confirm min/largest do not trend downward abnormally.

import string

def churn_once(n)
    var items = []
    var i = 0
    while i < n
        items.push("r5-item-" + str(i) + ":" + string.format("%04d", i))
        i += 1
    end
    var total = 0
    i = 0
    while i < size(items)
        total += size(items[i])
        i += 1
    end
    return total
end

def main(args)
    var round = 0
    while round < 5
        var bytes = churn_once(80)
        print("R5 churn round", round + 1, "chars", bytes)
        round += 1
    end

    print("R5 memory_churn done; run `berry gc` then `berry profile`")
    return 0
end
