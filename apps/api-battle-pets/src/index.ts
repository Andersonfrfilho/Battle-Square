// Copyright 2026 Anderson. All Rights Reserved.

import { environment } from './config/environment';
import {
  handleCreatePet,
  handleDeletePet,
  handleExportPets,
  handleGetPet,
  handleListPets,
  handleUpdatePet,
} from './pet/pet.controller';

const UUID_PATTERN = '[0-9a-fA-F-]{36}';
const petByIdPattern = new RegExp(`^/v1/pets/(${UUID_PATTERN})$`);

const server = Bun.serve({
  port: environment.PORT,
  async fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === '/health') {
      return Response.json({ data: { status: 'ok' } });
    }

    if (url.pathname === '/v1/pets/export') {
      if (request.method === 'GET') return handleExportPets(request);
    }

    if (url.pathname === '/v1/pets') {
      if (request.method === 'POST') return handleCreatePet(request);
      if (request.method === 'GET') return handleListPets(request);
    }

    const idMatch = url.pathname.match(petByIdPattern);
    if (idMatch) {
      const id = idMatch[1]!;
      if (request.method === 'GET') return handleGetPet(request, id);
      if (request.method === 'PUT') return handleUpdatePet(request, id);
      if (request.method === 'DELETE') return handleDeletePet(request, id);
    }

    return Response.json({ error: { code: 'NOT_FOUND', message: 'Rota não encontrada' } }, { status: 404 });
  },
});

console.log(`api-battle-pets ouvindo em http://localhost:${server.port}`);
