import { Actions } from "../../control/enums/ActionsEnum";

export type Params = {
  petName: string;
  type: string;
  level: number;
  health: number;
  attack: number;
  defense: number;
  speed: number;
  experience: number;
  ownerId?: string;
  actions?: Actions[];
};
