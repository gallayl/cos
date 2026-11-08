import ui from "@cos/ui";

type Brick = [number, number, boolean, number];

export default class Breakout implements CosApp {
  screen_width!: number;
  screen_height!: number;
  bricks!: Brick[];
  brick_count!: number;
  score!: number;
  lives!: number;
  game_state!: number;
  ball_x!: number;
  ball_y!: number;
  ball_dx!: number;
  ball_dy!: number;
  paddle_x!: number;
  touch_was_down!: boolean;
  keyboard_enabled!: boolean;

  setup(content: unknown, width: number, height: number): void {
    this.screen_width = width;
    this.screen_height = height;
    this.touch_was_down = false;
    this.keyboard_enabled = true;
    this.reset_game();
    ui.timer(20, () => {
      this.update_game();
    });
  }

  reset_game(): void {
    this.score = 0;
    this.lives = 3;
    this.game_state = 0;
    this.bricks = [];
    this.brick_count = 0;
    const colors = [
      ui.color565(255, 75, 90),
      ui.color565(255, 165, 55),
      ui.color565(250, 225, 75),
      ui.color565(80, 210, 125),
      ui.color565(85, 155, 255),
    ];
    let row = 0;
    while (row < 5) {
      let column = 0;
      while (column < 8) {
        const x = 9 + column * 38;
        const y = 42 + row * 15;
        this.bricks.push([x, y, true, colors[row]]);
        this.brick_count += 1;
        column += 1;
      }
      row += 1;
    }

    this.paddle_x = (this.screen_width - 52) / 2;
    this.ball_x = (this.screen_width - 6) / 2;
    this.ball_y = 190;
    this.ball_dx = ui.random(5)! - 2;
    if (this.ball_dx === 0) {
      this.ball_dx = 1;
    }
    this.ball_dy = -3;
    this.draw_scene();
  }

  draw_scene(): void {
    ui.start_write();
    ui.fill_screen(ui.color565(10, 16, 34));
    ui.fill_rect(0, 0, this.screen_width, 32, ui.color565(24, 35, 62));
    this.draw_hud();
    this.draw_bricks();
    ui.fill_rect(this.paddle_x, 210, 52, 7, ui.color565(90, 205, 255));
    ui.fill_rect(this.ball_x, this.ball_y, 6, 6, ui.color565(255, 255, 255));
    ui.draw_text(
      8,
      226,
      "Arrows / A,D or drag",
      ui.color565(160, 180, 210),
      ui.color565(10, 16, 34),
    );
    ui.end_write();
  }

  draw_hud(): void {
    ui.fill_rect(0, 0, this.screen_width, 32, ui.color565(24, 35, 62));
    ui.fill_rect(8, 4, 66, 24, ui.color565(48, 68, 105));
    ui.fill_rect(270, 4, 42, 24, ui.color565(120, 48, 58));
    ui.draw_text(12, 12, "RESET", ui.color565(255, 255, 255), ui.color565(48, 68, 105));
    ui.draw_text(276, 12, "EXIT", ui.color565(255, 255, 255), ui.color565(120, 48, 58));
    ui.draw_text(
      88,
      12,
      "Score " + str(this.score),
      ui.color565(255, 255, 255),
      ui.color565(24, 35, 62),
    );
    ui.draw_text(
      180,
      12,
      "Lives " + str(this.lives),
      ui.color565(255, 255, 255),
      ui.color565(24, 35, 62),
    );
  }

  draw_bricks(): void {
    let index = 0;
    while (index < this.bricks.length) {
      const brick = this.bricks[index];
      if (brick[2]) {
        ui.fill_rect(brick[0], brick[1], 35, 11, brick[3]);
      }
      index += 1;
    }
  }

  restore_bricks(x: number, y: number, width: number, height: number): void {
    let index = 0;
    while (index < this.bricks.length) {
      const brick = this.bricks[index];
      if (
        brick[2] &&
        x < brick[0] + 35 &&
        x + width > brick[0] &&
        y < brick[1] + 11 &&
        y + height > brick[1]
      ) {
        ui.fill_rect(brick[0], brick[1], 35, 11, brick[3]);
      }
      index += 1;
    }
  }

  update_game(): void {
    const old_paddle_x = this.paddle_x;
    const touch = ui.touch();
    if (touch === null) {
      this.touch_was_down = false;
    } else {
      const touch_x = touch[0];
      const touch_y = touch[1];
      if (!this.touch_was_down) {
        if (touch_y < 32 && touch_x >= 270) {
          log("Breakout: exit button clicked");
          ui.exit();
          return;
        } else if (touch_y < 32 && touch_x < 78) {
          log("Breakout: reset button clicked");
          this.reset_game();
          this.touch_was_down = true;
          return;
        }
      }
      this.touch_was_down = true;
      if (touch_y >= 32 && this.game_state === 0) {
        this.paddle_x = touch_x - 26;
        if (this.paddle_x < 0) {
          this.paddle_x = 0;
        } else if (this.paddle_x > this.screen_width - 52) {
          this.paddle_x = this.screen_width - 52;
        }
      }
    }

    if (touch === null && this.keyboard_enabled) {
      let direction = 0;
      if (ui.key_down(ui.KEY_LEFT) || ui.key_down(ui.KEY_A)) {
        direction = -1;
      } else if (ui.key_down(ui.KEY_RIGHT) || ui.key_down(ui.KEY_D)) {
        direction = 1;
      }
      this.paddle_x += direction * 5;
      if (this.paddle_x < 0) {
        this.paddle_x = 0;
      } else if (this.paddle_x > this.screen_width - 52) {
        this.paddle_x = this.screen_width - 52;
      }
    }

    if (this.game_state !== 0) {
      return;
    }

    const old_ball_x = this.ball_x;
    const old_ball_y = this.ball_y;
    const old_score = this.score;
    const old_lives = this.lives;
    this.ball_x += this.ball_dx;
    this.ball_y += this.ball_dy;

    if (this.ball_x <= 0) {
      this.ball_x = 0;
      this.ball_dx = -this.ball_dx;
    } else if (this.ball_x >= this.screen_width - 6) {
      this.ball_x = this.screen_width - 6;
      this.ball_dx = -this.ball_dx;
    }
    if (this.ball_y <= 33) {
      this.ball_y = 33;
      this.ball_dy = -this.ball_dy;
    }

    if (
      this.ball_dy > 0 &&
      this.ball_y + 6 >= 210 &&
      this.ball_y <= 217 &&
      this.ball_x + 6 >= this.paddle_x &&
      this.ball_x <= this.paddle_x + 52
    ) {
      this.ball_y = 204;
      this.ball_dy = -3;
      const offset = this.ball_x + 3 - (this.paddle_x + 26);
      this.ball_dx = offset / 9;
      if (this.ball_dx === 0) {
        if (offset < 0) {
          this.ball_dx = -1;
        } else {
          this.ball_dx = 1;
        }
      }
    }

    let index = 0;
    while (index < this.bricks.length) {
      const brick = this.bricks[index];
      if (
        brick[2] &&
        this.ball_x + 6 > brick[0] &&
        this.ball_x < brick[0] + 35 &&
        this.ball_y + 6 > brick[1] &&
        this.ball_y < brick[1] + 11
      ) {
        brick[2] = false;
        this.brick_count -= 1;
        this.score += 10;
        ui.fill_rect(brick[0], brick[1], 35, 11, ui.color565(10, 16, 34));
        this.ball_dy = -this.ball_dy;
        break;
      }
      index += 1;
    }

    if (this.ball_y > this.screen_height) {
      this.lives -= 1;
      if (this.lives <= 0) {
        this.game_state = 2;
      } else {
        this.ball_x = (this.screen_width - 6) / 2;
        this.ball_y = 190;
        this.ball_dx = ui.random(5)! - 2;
        if (this.ball_dx === 0) {
          this.ball_dx = 1;
        }
        this.ball_dy = -3;
      }
    }
    if (this.brick_count === 0) {
      this.game_state = 1;
    }

    ui.start_write();
    ui.fill_rect(old_ball_x, old_ball_y, 6, 6, ui.color565(10, 16, 34));
    this.restore_bricks(old_ball_x, old_ball_y, 6, 6);
    ui.fill_rect(old_paddle_x, 210, 52, 7, ui.color565(10, 16, 34));
    ui.fill_rect(this.paddle_x, 210, 52, 7, ui.color565(90, 205, 255));
    ui.fill_rect(this.ball_x, this.ball_y, 6, 6, ui.color565(255, 255, 255));

    if (this.score !== old_score || this.lives !== old_lives) {
      this.draw_hud();
    }
    if (this.game_state === 1) {
      ui.fill_rect(70, 105, 180, 30, ui.color565(24, 35, 62));
      ui.draw_text(96, 116, "YOU WIN! Tap RESET", ui.color565(255, 255, 255), ui.color565(24, 35, 62));
    } else if (this.game_state === 2) {
      ui.fill_rect(70, 105, 180, 30, ui.color565(24, 35, 62));
      ui.draw_text(92, 116, "GAME OVER - RESET", ui.color565(255, 255, 255), ui.color565(24, 35, 62));
    }
    ui.end_write();
  }
}
