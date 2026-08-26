// Copyright 2026 Anderson. All Rights Reserved.

import { environment } from './config/environment';
import {
  handleLogin,
  handleLogout,
  handleRefreshSession,
  handleRegisterAccount,
} from './account/account.controller';
import {
  handleBanAccount,
  handleLiftBan,
  handleListModerationHistory,
  handleRecordModerationEvent,
} from './moderation/moderation.controller';
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
const accountModerationEventsPattern = new RegExp(`^/v1/accounts/(${UUID_PATTERN})/moderation-events$`);
const accountBansPattern = new RegExp(`^/v1/accounts/(${UUID_PATTERN})/bans$`);
const banByIdPattern = new RegExp(`^/v1/bans/(${UUID_PATTERN})$`);

const server = Bun.serve({
  port: environment.PORT,

  /**
   * Barreira de erro global. Sem ela, uma exceção não tratada faz o Bun
   * devolver uma página de diagnóstico com a QUERY e os PARÂMETROS — foi
   * assim que um e-mail duplicado devolveu, ao cliente, o e-mail e o hash
   * Argon2 da senha. security.md §1 (PII) e a regra de nunca expor erro de
   * banco proíbem as duas coisas.
   *
   * O detalhe fica só no log do servidor; o cliente recebe código estável.
   */
  error(error: unknown) {
    console.error(
      JSON.stringify({
        level: 'error',
        message: 'unhandled_request_error',
        // Só o nome e a mensagem: params e query NUNCA saem daqui, nem para o log.
        errorName: error instanceof Error ? error.name : typeof error,
      }),
    );
    return Response.json(
      { error: { code: 'INTERNAL_ERROR', message: 'Erro interno' } },
      { status: 500 },
    );
  },

  async fetch(request) {
    const url = new URL(request.url);

    if (url.pathname === '/health') {
      return Response.json({ data: { status: 'ok' } });
    }

    if (url.pathname === '/v1/accounts') {
      if (request.method === 'POST') return handleRegisterAccount(request);
    }

    if (url.pathname === '/v1/accounts/sessions') {
      if (request.method === 'POST') return handleLogin(request);
      if (request.method === 'DELETE') return handleLogout(request);
    }

    if (url.pathname === '/v1/accounts/sessions/refresh') {
      if (request.method === 'POST') return handleRefreshSession(request);
    }

    const moderationEventsMatch = url.pathname.match(accountModerationEventsPattern);
    if (moderationEventsMatch) {
      const accountId = moderationEventsMatch[1]!;
      if (request.method === 'POST') return handleRecordModerationEvent(request, accountId);
      if (request.method === 'GET') return handleListModerationHistory(request, accountId);
    }

    const accountBansMatch = url.pathname.match(accountBansPattern);
    if (accountBansMatch) {
      if (request.method === 'POST') return handleBanAccount(request, accountBansMatch[1]!);
    }

    const banMatch = url.pathname.match(banByIdPattern);
    if (banMatch) {
      if (request.method === 'DELETE') return handleLiftBan(request, banMatch[1]!);
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
