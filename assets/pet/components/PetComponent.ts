// assets/pet/components/PetComponent.ts
import { Position } from "../../position/model/Position";
import { ActionsControls } from "../../control/ControlEnum";

export interface PetProps {
  name: string;
  type: string;
  attack: number;
  health: number;
  defense: number;
  experience: number;
  level: number;
  height: number;
  width: number;
  ownerId: string;
  position: Position;
  weight: number;
  speed: number;
  actions?: ActionsControls[];
}

export class PetComponent {
  public name: string;
  public type: string;
  public attack: number;
  public health: number;
  public defense: number;
  public experience: number;
  public level: number;
  public height: number;
  public width: number;
  public ownerId: string;
  public position: Position;
  public weight: number;
  public speed: number;
  public actions: ActionsControls[];

  constructor(props: PetProps) {
    this.name = props.name;
    this.type = props.type;
    this.attack = props.attack;
    this.health = props.health;
    this.defense = props.defense;
    this.experience = props.experience;
    this.level = props.level;
    this.height = props.height;
    this.width = props.width;
    this.ownerId = props.ownerId;
    this.position = props.position;
    this.weight = props.weight;
    this.speed = props.speed;
    this.actions = props.actions || [];
  }

  getBounds() {
    return {
      x: this.position.x,
      y: this.position.y,
      width: this.width,
      height: this.height,
    };
  }
}
