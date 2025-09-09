import { Actions } from "../../control/enums/ActionsEnum";
import { Params } from "./types";

export class PetModel {
  private readonly _petName: string;
  private readonly _type: string;
  private readonly _level: number;
  private readonly _health: number;
  private readonly _attack: number;
  private readonly _defense: number;
  private readonly _speed: number;
  private readonly _experience: number;
  private readonly _ownerId: string;
  private readonly _actions: Actions[];

  constructor({
    petName,
    type,
    level,
    health,
    attack,
    defense,
    speed,
    experience,
    ownerId = "",
    actions = [],
  }: Params) {
    this._petName = petName;
    this._type = type;
    this._level = level;
    this._health = health;
    this._attack = attack;
    this._defense = defense;
    this._speed = speed;
    this._experience = experience;
    this._ownerId = ownerId;
    this._actions = actions;
  }

  public get getName(): string {
    return this._petName;
  }
  public get petType(): string {
    return this._type;
  }
  public get petLevel(): number {
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
  public get actions(): Actions[] {
    return this._actions;
  }

  public set addActions(actions: Actions) {
    const index = this._actions.indexOf(undefined as unknown as Actions);
    if (index !== -1) {
      this._actions[index] = actions;
      this.node.emit("pet-actions-updated", this._actions);
      return;
    }

    this._actions.push(actions);

    this.node.emit("pet-actions-updated", this._actions);
  }

  public set removeActions(action: Actions) {
    const index = this._actions.indexOf(action);
    if (index !== -1) {
      this._actions.splice(index, 1);
      this.node.emit("pet-actions-updated", this._actions);
    }
  }

  applyActions(actions: Actions[]): void {
    actions.forEach((action) => {
      switch (action) {
        case Actions.ATTACK:
          console.log(`${this._petName} is attacking!`);
          break;
        case Actions.DEFEND:
          console.log(`${this._petName} is defending!`);
          break;
        case Actions.HEAL:
          console.log(`${this._petName} is healing!`);
          break;
        default:
          console.log(`${this._petName} does nothing.`);
      }
    });
  }

  getDamageFromAction(
    action: Actions,
    target: PetModel,
    targetHpBefore: number
  ): number {
    switch (action) {
      case Actions.ATTACK:
        const baseDamage = this._attack - target.defense;
        return Math.max(1, baseDamage); // dano mínimo de 1
      case Actions.DEFEND:
        return 0; // não causa dano
      case Actions.HEAL:
        return 0; // não causa dano
      default:
        return 0; // ação desconhecida não causa dano
    }
  }
}
