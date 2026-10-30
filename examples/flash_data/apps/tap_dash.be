class TapDash
    var score, seconds, lane, screen_width, screen_height
    var score_label, time_label, status_label, target

    def setup(content, width, height)
        self.score = 0
        self.seconds = 20
        self.lane = 1
        self.screen_width = width
        self.screen_height = height

        self.score_label = ui.label(0, "Score: 0", 16, 16, 120, 20)
        self.time_label = ui.label(0, "Time: 20", width - 112, 16, 96, 20)
        self.status_label = ui.label(0, "Tap the moving target!", 16, 42, width - 32, 20)
        self.target = ui.button(0, "TAP!", (width - 88) / 2, (height - 60) / 2, 88, 60)
        var restart = ui.button(0, "RESTART", (width - 120) / 2, height - 58, 120, 40)

        ui.on_click(self.target, def () self.tap_target() end)
        ui.on_click(restart, def () self.restart_game() end)
        ui.timer(1000, def () self.update_game_clock() end)
    end

    def tap_target()
        log("Tap Dash: target button clicked")
        if self.seconds > 0
            self.score += 1
            self.lane = (self.lane + 1) % 3
            ui.set_text(self.score_label, "Score: " + str(self.score))
            ui.set_bounds(self.target, self.lane * (self.screen_width - 88) / 2,
                          (self.screen_height - 60) / 2, 88, 60)
        end
    end

    def update_game_clock()
        if self.seconds > 0
            self.seconds -= 1
            ui.set_text(self.time_label, "Time: " + str(self.seconds))
            if self.seconds == 0
                ui.set_text(self.target, "DONE")
                ui.set_text(self.status_label, "Final score: " + str(self.score))
            end
        end
    end

    def restart_game()
        log("Tap Dash: restart button clicked")
        self.score = 0
        self.seconds = 20
        self.lane = 1
        ui.set_text(self.score_label, "Score: 0")
        ui.set_text(self.time_label, "Time: 20")
        ui.set_text(self.status_label, "Tap the moving target!")
        ui.set_text(self.target, "TAP!")
        ui.set_bounds(self.target, (self.screen_width - 88) / 2,
                      (self.screen_height - 60) / 2, 88, 60)
    end
end

return TapDash
