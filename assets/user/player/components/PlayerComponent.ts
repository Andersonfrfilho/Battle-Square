// assets/player/components/PlayerComponent.ts

import { PetComponent } from "../../../pet/components/PetComponent";

export interface PlayerProps {
  name: string;
  health: number;
  pets: PetComponent[];
}

export class PlayerComponent {
  public name: string;
  public health: number;
  public pets: PetComponent[];

  constructor(props: PlayerProps) {
    this.name = props.name;
    this.health = props.health;
    this.pets = props.pets;
  }
}
