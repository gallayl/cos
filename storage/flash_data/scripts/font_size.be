var __cos_mod_1 = cos_import("@cos/console")
var console = __cos_mod_1.find("default")
class FontSize
    def run(args)
        if size(args) != 1
            print("usage: font_size 1|2|3")
            return 1
        end
        var requested = args[0]
        var result = "ESP_ERR_INVALID_ARG"
        if requested == "1"
            result = console.set_font_size(1)
        elif requested == "2"
            result = console.set_font_size(2)
        elif requested == "3"
            result = console.set_font_size(3)
        else
            print("font size must be 1, 2, or 3")
            return 1
        end
        if result != true
            print(str(result))
            return 1
        end
        print("font size " + str(console.font_size()))
        return 0
    end
end

var __cos_entry = FontSize()
