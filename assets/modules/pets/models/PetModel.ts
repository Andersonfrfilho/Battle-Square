import { MAX_ACTIONS_BY_PET } from "../../commons/constants";
import { ActionsControls } from "../../control/ControlEnum";
import {
  ActionsPet,
  AddActionProps,
  Params,
  RemoveActionsProps,
} from "./types";

export type Position = { x: number; y: number };
export class PetModel {
  private readonly _name: string;
  private readonly _type: string;
  private readonly _level: number;
  private readonly _health: number;
  private readonly _attack: number;
  private readonly _defense: number;
  private readonly _speed: number;
  private readonly _experience: number;
  private readonly _ownerId: string;
  private readonly _actions: ActionsPet;
  private readonly _position: Position;
  private readonly _width: number = 50;
  private readonly _height: number = 50;
  private readonly _weight: number = 1;

  constructor({
    name,
    type,
    level,
    health,
    attack,
    defense,
    speed,
    experience,
    ownerId = "",
    actions = [],
    position = { x: 0, y: 0 },
    height = 50,
    width = 50,
    weight = 1,
  }: Params) {
    this._name = name;
    this._type = type;
    this._level = level;
    this._health = health;
    this._attack = attack;
    this._defense = defense;
    this._speed = speed;
    this._experience = experience;
    this._ownerId = ownerId;
    this._actions = actions;
    this._position = position;
    this._height = height;
    this._width = width;
    this._weight = weight;
  }

  public get name(): string {
    return this._name;
  }
  public get type(): string {
    return this._type;
  }
  public get level(): number {
    return this._level;
  }
  public get health(): number {
    return this._health;
  }
  public get attack(): number {
    return this._attack;
  }
  public get defense(): number {
    return this._defense;
  }
  public get speed(): number {
    return this._speed;
  }
  public get experience(): number {
    return this._experience;
  }
  public get ownerId(): string {
    return this._ownerId;
  }
  public get actions(): ActionsPet {
    return this._actions;
  }
  public get position(): Position {
    return this._position;
  }
  public get width(): number {
    return this._width;
  }
  public get height(): number {
    return this._height;
  }
  public get weight(): number {
    return this._weight;
  }

  public set position(position: Position) {
    this._position.x = position.x;
    this._position.y = position.y;
  }

  public addAction(action: AddActionProps): boolean {
    if (this.limitActions) return false;

    const index = this._actions.indexOf(undefined);
    if (index !== -1) {
      this._actions[index] = action;
      // this.node.emit("pet-actions-updated", this._actions);
      return true;
    }

    this._actions.push(action);

    // this.node.emit("pet-actions-updated", this._actions);
    return true;
  }
  public get limitActions(): boolean {
    return this._actions.length >= MAX_ACTIONS_BY_PET;
  }

  public set removeActions(action: RemoveActionsProps) {
    const index = this._actions.indexOf(action);
    if (index !== -1) {
      this._actions.splice(index, 1);
      // this.node.emit("pet-actions-updated", this._actions);
    }
  }

  public clearActions(): void {
    this._actions.splice(0);
  }

  getBounds() {
    return {
      x: this.position.x,
      y: this.position.y,
      width: this.width,
      height: this.height,
    };
  }

  // getDamageFromAction(
  //   action: Actions,
  //   target: PetModel,
  //   targetHpBefore: number
  // ): number {
  //   switch (action) {
  //     case Actions.ATTACK:
  //       const baseDamage = this._attack - target.defense;
  //       return Math.max(1, baseDamage); // dano mínimo de 1
  //     case Actions.DEFEND:
  //       return 0; // não causa dano
  //     case Actions.HEAL:
  //       return 0; // não causa dano
  //     default:
  //       return 0; // ação desconhecida não causa dano
  //   }
  // }
}
