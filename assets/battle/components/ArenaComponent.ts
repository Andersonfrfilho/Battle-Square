import { PositionComponent } from "../../position/components/PositionComponent";

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

export type GetNextPositionParams = {
  position: PositionComponent;
  direction: { x: number; y: number };
};

export class ArenaComponent {
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

  isInsideBounds(position: { x: number; y: number }): boolean {
    return !!this.getBlockAt(position);
  }

  isInRange(
    posA: { x: number; y: number },
    posB: { x: number; y: number },
    range: number
  ): boolean {
    const dx = posA.x - posB.x;
    const dy = posA.y - posB.y;
    return Math.sqrt(dx * dx + dy * dy) <= range;
  }

  getNextPosition({ position, direction }: GetNextPositionParams) {
    return { x: position.x + direction.x, y: position.y + direction.y };
  }

  private getBlockAt(position: { x: number; y: number }) {
    return this.blocks
      .flat()
      .find(
        (b) =>
          position.x >= b.x &&
          position.x < b.x + b.width &&
          position.y >= b.y &&
          position.y < b.y + b.height
      );
  }
}
