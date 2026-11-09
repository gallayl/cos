// Runs once during shell startup. Edit or remove this file to customize startup.
export default class Autoexec implements CosApplication {
  run(_args: string[]): number {
    action("clear");
    return 0;
  }
}
