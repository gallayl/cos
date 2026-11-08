import ui from "@cos/ui";

export default class TapDash implements CosApp {
  score!: number;
  seconds!: number;
  screen_width!: number;
  screen_height!: number;
  score_label!: ui.LabelHandle;
  time_label!: ui.LabelHandle;
  status_label!: ui.LabelHandle;
  target!: ui.ButtonHandle;

  setup(content: unknown, width: number, height: number): void {
    this.score = 0;
    this.seconds = 20;
    this.screen_width = width;
    this.screen_height = height;

    this.score_label = ui.label(0, "Score: 0", 16, 16, 120, 20)!;
    this.time_label = ui.label(0, "Time: 20", width - 112, 16, 96, 20)!;
    this.status_label = ui.label(0, "Tap the moving target!", 16, 42, width - 32, 20)!;
    this.target = ui.button(0, "TAP!", (width - 88) / 2, (height - 60) / 2, 88, 60)!;
    const exit = ui.button(0, "EXIT", 16, height - 48, 88, 36)!;
    const restart = ui.button(0, "RESTART", width - 120, height - 48, 104, 36)!;

    ui.on_click(this.target, () => {
      this.tap_target();
    });
    ui.on_click(exit, () => {
      this.exit_game();
    });
    ui.on_click(restart, () => {
      this.restart_game();
    });
    this.move_target();
    ui.timer(1000, () => {
      this.update_game_clock();
    });
  }

  move_target(): void {
    const max_x = this.screen_width - 96;
    const max_y = this.screen_height - 128;
    const x = 8 + ui.random(max_x - 7)!;
    const y = 72 + ui.random(max_y - 71)!;
    ui.set_bounds(this.target, x, y, 88, 60);
  }

  tap_target(): void {
    log("Tap Dash: target button clicked");
    if (this.seconds > 0) {
      this.score += 1;
      ui.set_text(this.score_label, "Score: " + str(this.score));
      this.move_target();
    }
  }

  update_game_clock(): void {
    if (this.seconds > 0) {
      this.seconds -= 1;
      ui.set_text(this.time_label, "Time: " + str(this.seconds));
      if (this.seconds === 0) {
        ui.set_text(this.target, "DONE");
        ui.set_text(this.status_label, "Final score: " + str(this.score));
      }
    }
  }

  restart_game(): void {
    log("Tap Dash: restart button clicked");
    this.score = 0;
    this.seconds = 20;
    ui.set_text(this.score_label, "Score: 0");
    ui.set_text(this.time_label, "Time: 20");
    ui.set_text(this.status_label, "Tap the moving target!");
    ui.set_text(this.target, "TAP!");
    this.move_target();
  }

  exit_game(): void {
    log("Tap Dash: exit button clicked");
    ui.exit();
  }
}
