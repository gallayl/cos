var game_score = 0
var game_seconds = 20
var game_lane = 1
var game_width = 320
var game_height = 240
var game_score_label = nil
var game_time_label = nil
var game_status_label = nil
var game_target = nil

def tap_target()
    if game_seconds > 0
        game_score += 1
        game_lane = (game_lane + 1) % 3
        ui.set_text(game_score_label, "Score: " + str(game_score))
        ui.set_bounds(game_target, game_lane * (game_width - 88) / 2, (game_height - 60) / 2, 88, 60)
    end
end

def update_game_clock()
    if game_seconds > 0
        game_seconds -= 1
        ui.set_text(game_time_label, "Time: " + str(game_seconds))
        if game_seconds == 0
            ui.set_text(game_target, "DONE")
            ui.set_text(game_status_label, "Final score: " + str(game_score))
        end
    end
end

def restart_game()
    game_score = 0
    game_seconds = 20
    game_lane = 1
    ui.set_text(game_score_label, "Score: 0")
    ui.set_text(game_time_label, "Time: 20")
    ui.set_text(game_status_label, "Tap the moving target!")
    ui.set_text(game_target, "TAP!")
    ui.set_bounds(game_target, (game_width - 88) / 2, (game_height - 60) / 2, 88, 60)
end

class TapDash
    def setup(content, width, height)
        game_width = width
        game_height = height
        game_score = 0
        game_seconds = 20
        game_lane = 1

        game_score_label = ui.label(0, "Score: 0", 16, 16, 120, 20)
        game_time_label = ui.label(0, "Time: 20", width - 112, 16, 96, 20)
        game_status_label = ui.label(0, "Tap the moving target!", 16, 42, width - 32, 20)
        game_target = ui.button(0, "TAP!", (width - 88) / 2, (height - 60) / 2, 88, 60)
        var restart = ui.button(0, "RESTART", (width - 120) / 2, height - 58, 120, 40)

        ui.on_click(game_target, tap_target)
        ui.on_click(restart, restart_game)
        ui.timer(1000, update_game_clock)
    end
end

return TapDash
