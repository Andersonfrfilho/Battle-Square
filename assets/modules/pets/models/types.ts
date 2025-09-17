import { ActionsControls } from "../../control/ControlEnum";

export type ActionPet = ActionsControls;
export type ActionsPet = ActionPet[];

export type Position = { x: number; y: number };

export type Params = {
  name: string;
  type: string;
  level: number;
  health: number;
  attack: number;
  defense: number;
  speed: number;
  experience: number;
  ownerId?: string;
  actions?: ActionsPet;
  position?: Position;
  width?: number;
  height?: number;
  weight?: number;
};

export type AddActionProps = ActionPet;
export type RemoveActionsProps = ActionPet;
export type ApplyActionsProps = ActionsPet;
