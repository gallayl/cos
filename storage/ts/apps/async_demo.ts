import ui from "@cos/ui";

export default class AsyncDemo implements CosUiApplication {
  status_label!: ui.LabelHandle;

  setup(_content: unknown, width: number, _height: number): void {
    this.status_label = ui.label(0, "Async demo: starting...", 16, 16, width - 32, 24)!;
    const exit = ui.button(0, "EXIT", 16, 56, 88, 36)!;
    ui.on_click(exit, () => {
      ui.exit();
    });
    this.run_demo();
  }

  async run_demo(): Promise<void> {
    await system.sleep(1000);
    ui.set_text(this.status_label, "Async demo: slept 1000ms");
    await system.sleep(1000);
    ui.set_text(this.status_label, "Async demo: done");
  }
}
