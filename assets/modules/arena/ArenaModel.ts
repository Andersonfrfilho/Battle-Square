export type ArenaBlock = {
  x: number;
  y: number;
  width: number;
  height: number;
};

export type ArenaModelProps = {
  size?: number;
  blockSize: number;
  initialPosition?: { x: number; y: number };
};
export class ArenaModel {
  private readonly size: number;
  private readonly blocks: ArenaBlock[][];
  private readonly initialPosition = { x: 0, y: 0 };

  constructor({ size = 10, blockSize, initialPosition }: ArenaModelProps) {
    this.size = size;
    this.blocks = [];
    this.initialPosition = initialPosition;

    for (let row = 0; row < size; row++) {
      this.blocks[row] = [];
      for (let col = 0; col < size; col++) {
        this.blocks[row][col] = {
          x: this.initialPosition.x + col * blockSize,
          y: this.initialPosition.y + row * blockSize,
          width: blockSize,
          height: blockSize,
        };
      }
    }
  }

  public getBlock(row: number, col: number): ArenaBlock {
    return this.blocks[row][col];
  }

  public getAllBlocks(): ArenaBlock[][] {
    return this.blocks;
  }
}
