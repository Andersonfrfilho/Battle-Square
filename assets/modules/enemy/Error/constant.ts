const CODE_ERROR = {
  ENEMY_HAS_NO_PETS: 50001,
};
export const ENEMY_ERRORS = {
  ENEMY_HAS_NO_PETS: {
    code: CODE_ERROR.ENEMY_HAS_NO_PETS,
    message: "Enemy has no pets",
    details: "The enemy does not have any pets available.",
    metadata: {
      timestamp: new Date().toISOString(),
    },
  },
};
