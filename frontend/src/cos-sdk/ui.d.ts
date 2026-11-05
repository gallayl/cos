declare namespace ui {
  function fill_screen(color: number): void;

  function fill_rect(
    x: number,
    y: number,
    width: number,
    height: number,
    color: number,
  ): void;

  function draw_text(
    x: number,
    y: number,
    text: string,
    color: number,
    background: number,
  ): void;

  function timer(interval: number, callback: () => void): void;

  function touch(): [number, number] | null;
}

declare function log(message: string): void;

declare function action(command: string): number;
