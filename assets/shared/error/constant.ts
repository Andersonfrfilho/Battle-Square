const CODE_ERROR_COMMONS = {
  INVALID_PET_INDEX: 10001,
};
export const ERROR = {
  INVALID_PET_INDEX: {
    code: CODE_ERROR_COMMONS.INVALID_PET_INDEX,
    message: "Invalid pet index",
    details: "The pet index provided is invalid.",
    metadata: {
      timestamp: new Date().toISOString(),
    },
  },
};
