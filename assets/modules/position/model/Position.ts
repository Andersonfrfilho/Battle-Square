export class Position {
  constructor(public x: number, public y: number) {}

  public setPosition(x: number, y: number) {
    this.x = x;
    this.y = y;
  }
}
