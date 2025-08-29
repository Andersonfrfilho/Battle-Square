// assets/shared/constants/Config.ts
export const Config = {
  MAX_ACTIONS_PER_PET: 3, // Parametrizável via JSON ou UI no futuro
  GRID_SIZE: 3, // Grid 3x3
  ACTION_TYPES: {
    UP: "UP",
    DOWN: "DOWN",
    LEFT: "LEFT",
    RIGHT: "RIGHT",
    LEFT_UP: "LEFT_UP",
    LEFT_DOWN: "LEFT_DOWN",
    RIGHT_UP: "RIGHT_UP",
    RIGHT_DOWN: "RIGHT_DOWN",
    DODGE: "DODGE",
    ATTACK: "ATTACK",
    MAGIC: "MAGIC",
    DEFEND: "DEFEND",
  },
};
