class Breakout
    var screen_width, screen_height
    var bricks, brick_count, score, lives, game_state
    var ball_x, ball_y, ball_dx, ball_dy, paddle_x
    var touch_was_down, keyboard_enabled

    def setup(content, width, height)
        self.screen_width = width
        self.screen_height = height
        self.touch_was_down = false
        self.keyboard_enabled = true
        self.reset_game()
        ui.timer(20, def () self.update_game() end)
    end

    def reset_game()
        self.score = 0
        self.lives = 3
        self.game_state = 0
        self.bricks = []
        self.brick_count = 0
        var colors = [ui.color565(255, 75, 90), ui.color565(255, 165, 55),
                      ui.color565(250, 225, 75), ui.color565(80, 210, 125),
                      ui.color565(85, 155, 255)]
        var row = 0
        while row < 5
            var column = 0
            while column < 8
                var x = 9 + column * 38
                var y = 42 + row * 15
                self.bricks.push([x, y, true, colors[row]])
                self.brick_count += 1
                column += 1
            end
            row += 1
        end

        self.paddle_x = (self.screen_width - 52) / 2
        self.ball_x = (self.screen_width - 6) / 2
        self.ball_y = 190
        self.ball_dx = ui.random(5) - 2
        if self.ball_dx == 0
            self.ball_dx = 1
        end
        self.ball_dy = -3
        self.draw_scene()
    end

    def draw_scene()
        ui.start_write()
        ui.fill_screen(ui.color565(10, 16, 34))
        ui.fill_rect(0, 0, self.screen_width, 32, ui.color565(24, 35, 62))
        self.draw_hud()
        self.draw_bricks()
        ui.fill_rect(self.paddle_x, 210, 52, 7, ui.color565(90, 205, 255))
        ui.fill_rect(self.ball_x, self.ball_y, 6, 6, ui.color565(255, 255, 255))
        ui.draw_text(8, 226, "Arrows / A,D or drag", ui.color565(160, 180, 210),
                     ui.color565(10, 16, 34))
        ui.end_write()
    end

    def draw_hud()
        ui.fill_rect(0, 0, self.screen_width, 32, ui.color565(24, 35, 62))
        ui.fill_rect(8, 4, 66, 24, ui.color565(48, 68, 105))
        ui.fill_rect(270, 4, 42, 24, ui.color565(120, 48, 58))
        ui.draw_text(12, 12, "RESET", ui.color565(255, 255, 255), ui.color565(48, 68, 105))
        ui.draw_text(276, 12, "EXIT", ui.color565(255, 255, 255), ui.color565(120, 48, 58))
        ui.draw_text(88, 12, "Score " + str(self.score), ui.color565(255, 255, 255),
                     ui.color565(24, 35, 62))
        ui.draw_text(180, 12, "Lives " + str(self.lives), ui.color565(255, 255, 255),
                     ui.color565(24, 35, 62))
    end

    def draw_bricks()
        var index = 0
        while index < self.bricks.size()
            var brick = self.bricks[index]
            if brick[2]
                ui.fill_rect(brick[0], brick[1], 35, 11, brick[3])
            end
            index += 1
        end
    end

    def restore_bricks(x, y, width, height)
        var index = 0
        while index < self.bricks.size()
            var brick = self.bricks[index]
            if brick[2] && x < brick[0] + 35 && x + width > brick[0] &&
               y < brick[1] + 11 && y + height > brick[1]
                ui.fill_rect(brick[0], brick[1], 35, 11, brick[3])
            end
            index += 1
        end
    end

    def update_game()
        var old_paddle_x = self.paddle_x
        var touch = ui.touch()
        if touch == nil
            self.touch_was_down = false
        else
            var touch_x = touch[0]
            var touch_y = touch[1]
            if !self.touch_was_down
                if touch_y < 32 && touch_x >= 270
                    log("Breakout: exit button clicked")
                    ui.exit()
                    return
                elif touch_y < 32 && touch_x < 78
                    log("Breakout: reset button clicked")
                    self.reset_game()
                    self.touch_was_down = true
                    return
                end
            end
            self.touch_was_down = true
            if touch_y >= 32 && self.game_state == 0
                self.paddle_x = touch_x - 26
                if self.paddle_x < 0
                    self.paddle_x = 0
                elif self.paddle_x > self.screen_width - 52
                    self.paddle_x = self.screen_width - 52
                end
            end
        end

        if touch == nil && self.keyboard_enabled
            var direction = 0
            if ui.key_down(ui.KEY_LEFT) || ui.key_down(ui.KEY_A)
                direction = -1
            elif ui.key_down(ui.KEY_RIGHT) || ui.key_down(ui.KEY_D)
                direction = 1
            end
            self.paddle_x += direction * 5
            if self.paddle_x < 0
                self.paddle_x = 0
            elif self.paddle_x > self.screen_width - 52
                self.paddle_x = self.screen_width - 52
            end
        end

        if self.game_state != 0
            return
        end

        var old_ball_x = self.ball_x
        var old_ball_y = self.ball_y
        var old_score = self.score
        var old_lives = self.lives
        self.ball_x += self.ball_dx
        self.ball_y += self.ball_dy

        if self.ball_x <= 0
            self.ball_x = 0
            self.ball_dx = -self.ball_dx
        elif self.ball_x >= self.screen_width - 6
            self.ball_x = self.screen_width - 6
            self.ball_dx = -self.ball_dx
        end
        if self.ball_y <= 33
            self.ball_y = 33
            self.ball_dy = -self.ball_dy
        end

        if self.ball_dy > 0 && self.ball_y + 6 >= 210 &&
           self.ball_y <= 217 && self.ball_x + 6 >= self.paddle_x &&
           self.ball_x <= self.paddle_x + 52
            self.ball_y = 204
            self.ball_dy = -3
            var offset = self.ball_x + 3 - (self.paddle_x + 26)
            self.ball_dx = offset / 9
            if self.ball_dx == 0
                if offset < 0
                    self.ball_dx = -1
                else
                    self.ball_dx = 1
                end
            end
        end

        var index = 0
        while index < self.bricks.size()
            var brick = self.bricks[index]
            if brick[2] && self.ball_x + 6 > brick[0] &&
               self.ball_x < brick[0] + 35 && self.ball_y + 6 > brick[1] &&
               self.ball_y < brick[1] + 11
                brick[2] = false
                self.brick_count -= 1
                self.score += 10
                ui.fill_rect(brick[0], brick[1], 35, 11, ui.color565(10, 16, 34))
                self.ball_dy = -self.ball_dy
                break
            end
            index += 1
        end

        if self.ball_y > self.screen_height
            self.lives -= 1
            if self.lives <= 0
                self.game_state = 2
            else
                self.ball_x = (self.screen_width - 6) / 2
                self.ball_y = 190
                self.ball_dx = ui.random(5) - 2
                if self.ball_dx == 0
                    self.ball_dx = 1
                end
                self.ball_dy = -3
            end
        end
        if self.brick_count == 0
            self.game_state = 1
        end

        ui.start_write()
        ui.fill_rect(old_ball_x, old_ball_y, 6, 6, ui.color565(10, 16, 34))
        self.restore_bricks(old_ball_x, old_ball_y, 6, 6)
        ui.fill_rect(old_paddle_x, 210, 52, 7, ui.color565(10, 16, 34))
        ui.fill_rect(self.paddle_x, 210, 52, 7, ui.color565(90, 205, 255))
        ui.fill_rect(self.ball_x, self.ball_y, 6, 6, ui.color565(255, 255, 255))

        if self.score != old_score || self.lives != old_lives
            self.draw_hud()
        end
        if self.game_state == 1
            ui.fill_rect(70, 105, 180, 30, ui.color565(24, 35, 62))
            ui.draw_text(96, 116, "YOU WIN! Tap RESET", ui.color565(255, 255, 255),
                         ui.color565(24, 35, 62))
        elif self.game_state == 2
            ui.fill_rect(70, 105, 180, 30, ui.color565(24, 35, 62))
            ui.draw_text(92, 116, "GAME OVER - RESET", ui.color565(255, 255, 255),
                         ui.color565(24, 35, 62))
        end
        ui.end_write()
    end
end

return Breakout
